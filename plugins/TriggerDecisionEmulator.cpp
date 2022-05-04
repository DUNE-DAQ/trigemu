/**
 * @file TriggerDecisionEmulator.cpp TriggerDecisionEmulator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TriggerDecisionEmulator.hpp"

#include "daqdataformats/ComponentRequest.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "dfmessages/TimeSync.hpp"
#include "dfmessages/TriggerDecision.hpp"
#include "dfmessages/TriggerInhibit.hpp"
#include "dfmessages/Types.hpp"
#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"

#include "trigemu/Issues.hpp"
#include "trigemu/TimestampEstimator.hpp"
#include "trigemu/triggerdecisionemulator/Nljs.hpp"
#include "trigemu/triggerdecisionemulatorinfo/InfoNljs.hpp"

#include "appfwk/app/Nljs.hpp"
#include "rcif/cmd/Nljs.hpp"

#include <algorithm>
#include <cassert>
#include <pthread.h>
#include <random>
#include <string>
#include <vector>

namespace dunedaq {
namespace trigemu {

TriggerDecisionEmulator::TriggerDecisionEmulator(const std::string& name)
  : DAQModule(name)
  , m_time_sync_source(nullptr)
  , m_trigger_inhibit_source(nullptr)
  , m_token_source(nullptr)
  , m_trigger_decision_sink(nullptr)
  , m_inhibited(false)
  , m_last_trigger_number(0)
  , m_run_number(0)
{
  register_command("conf", &TriggerDecisionEmulator::do_configure);
  register_command("start", &TriggerDecisionEmulator::do_start);
  register_command("stop", &TriggerDecisionEmulator::do_stop);
  register_command("pause", &TriggerDecisionEmulator::do_pause);
  register_command("resume", &TriggerDecisionEmulator::do_resume);
  register_command("scrap", &TriggerDecisionEmulator::do_scrap);
}

void
TriggerDecisionEmulator::init(const nlohmann::json& iniobj)
{
  auto ini = iniobj.get<appfwk::app::ModInit>();
  auto qi = appfwk::connection_index(
    iniobj, { "time_sync_source", "trigger_inhibit_source", "trigger_decision_sink", "token_source" });
  m_time_sync_source = get_iom_receiver<dfmessages::TimeSync>(qi["time_sync_source"]);
  m_trigger_inhibit_source = get_iom_receiver<dfmessages::TriggerInhibit>(qi["trigger_inhibit_source"]);
  m_trigger_decision_sink = get_iom_sender<dfmessages::TriggerDecision>(qi["trigger_decision_sink"]);
  m_token_source = get_iom_receiver<dfmessages::TriggerDecisionToken>(qi["token_source"]);
}

void
TriggerDecisionEmulator::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  triggerdecisionemulatorinfo::Info tde;

  tde.triggers = m_trigger_count_tot.load();
  tde.new_triggers = m_trigger_count.exchange(0);
  tde.inhibited = m_inhibited_trigger_count_tot.load();
  tde.new_inhibited = m_inhibited_trigger_count.exchange(0);

  ci.add(tde);
}

void
TriggerDecisionEmulator::do_configure(const nlohmann::json& confobj)
{
  auto params = confobj.get<triggerdecisionemulator::ConfParams>();

  m_min_readout_window_ticks = params.min_readout_window_ticks;
  m_max_readout_window_ticks = params.max_readout_window_ticks;
  m_trigger_window_offset = params.trigger_window_offset;

  m_min_links_in_request = params.min_links_in_request;
  m_max_links_in_request = params.max_links_in_request;

  m_trigger_interval_ticks.store(params.trigger_interval_ticks);

  m_trigger_offset = params.trigger_offset;
  trigger_delay_ticks_ = params.trigger_delay_ticks;
  m_clock_frequency_hz = params.clock_frequency_hz;
  m_repeat_trigger_count = params.repeat_trigger_count;

  m_stop_burst_count = params.stop_burst_count;
  m_initial_tokens = params.initial_token_count;

  m_links.clear();
  for (auto const& link : params.links) {
    // For the future: Set APA properly
    m_links.push_back(
      dfmessages::GeoID{ dfmessages::GeoID::SystemType::kTPC, 0, static_cast<uint32_t>(link) }); // NOLINT
  }

  // Sanity-check the values
  if (m_min_readout_window_ticks > m_max_readout_window_ticks || m_min_links_in_request > m_max_links_in_request) {
    throw InvalidConfiguration(ERS_HERE);
  }

  m_configured_flag.store(true);
}

void
TriggerDecisionEmulator::do_start(const nlohmann::json& startobj)
{
  auto start_pars = startobj.get<dunedaq::rcif::cmd::StartParams>();
  m_run_number = start_pars.run;

  if (start_pars.trigger_interval_ticks <= 0) {
    throw InvalidTriggerInterval(ERS_HERE, start_pars.trigger_interval_ticks);
  }

  m_paused.store(true);
  m_inhibited.store(false);
  m_running_flag.store(true);

  m_tokens.store(m_initial_tokens);
  m_open_trigger_decisions.clear();

  m_timestamp_estimator.reset(new TimestampEstimator(m_time_sync_source, m_clock_frequency_hz));

  m_read_inhibit_queue_thread = std::thread(&TriggerDecisionEmulator::read_inhibit_queue, this);
  pthread_setname_np(m_read_inhibit_queue_thread.native_handle(), "tde-inhibit-q");

  m_read_token_queue_thread = std::thread(&TriggerDecisionEmulator::read_token_queue, this);
  pthread_setname_np(m_read_token_queue_thread.native_handle(), "tde-token-q");

  m_send_trigger_decisions_thread = std::thread(&TriggerDecisionEmulator::send_trigger_decisions, this);
  pthread_setname_np(m_send_trigger_decisions_thread.native_handle(), "tde-trig-dec");
}

void
TriggerDecisionEmulator::do_stop(const nlohmann::json& /*stopobj*/)
{
  m_running_flag.store(false);

  m_read_inhibit_queue_thread.join();
  m_read_token_queue_thread.join();
  m_send_trigger_decisions_thread.join();

  m_timestamp_estimator.reset(nullptr); // Calls TimestampEstimator dtor
}

void
TriggerDecisionEmulator::do_pause(const nlohmann::json& /*pauseobj*/)
{
  m_paused.store(true);
  TLOG() << "******* Triggers PAUSED! *********";
}

void
TriggerDecisionEmulator::do_resume(const nlohmann::json& resumeobj)
{
  auto params = resumeobj.get<triggerdecisionemulator::ResumeParams>();
  if (params.trigger_interval_ticks <= 0) {
    throw InvalidTriggerInterval(ERS_HERE, params.trigger_interval_ticks);
  }
  m_trigger_interval_ticks.store(params.trigger_interval_ticks);

  TLOG() << "******* Triggers RESUMED! *********";
  m_paused.store(false);
}

void
TriggerDecisionEmulator::do_scrap(const nlohmann::json& /*stopobj*/)
{
  m_configured_flag.store(false);
}

dfmessages::TriggerDecision
TriggerDecisionEmulator::create_decision(dfmessages::timestamp_t timestamp)
{
  static std::default_random_engine random_engine(m_run_number);
  static std::uniform_int_distribution<int> n_links_dist(m_min_links_in_request,
                                                         std::min((size_t)m_max_links_in_request, m_links.size()));
  static std::uniform_int_distribution<dfmessages::timestamp_t> window_ticks_dist(m_min_readout_window_ticks,
                                                                                  m_max_readout_window_ticks);

  dfmessages::TriggerDecision decision;
  decision.trigger_number = m_last_trigger_number + 1;
  decision.run_number = m_run_number;
  decision.trigger_timestamp = timestamp;
  decision.trigger_type = m_trigger_type;

  int n_links = n_links_dist(random_engine);

  std::vector<dfmessages::GeoID> this_links;
  std::sample(m_links.begin(), m_links.end(), std::back_inserter(this_links), n_links, random_engine);

  for (auto link : this_links) {
    dfmessages::ComponentRequest request;
    request.component = link;
    request.window_begin = timestamp - m_trigger_window_offset;
    request.window_end = request.window_begin + window_ticks_dist(random_engine);

    decision.components.push_back(request);
  }

  return decision;
}

void
TriggerDecisionEmulator::send_trigger_decisions()
{

  // We get here at start of run, so reset the trigger number
  m_last_trigger_number = 0;
  m_trigger_count.store(0);
  m_trigger_count_tot.store(0);
  m_inhibited_trigger_count.store(0);
  m_inhibited_trigger_count_tot.store(0);

  // Wait for there to be a valid timestamp estimate before we start
  while (m_running_flag.load() &&
         m_timestamp_estimator->get_timestamp_estimate() == dfmessages::TypeDefaults::s_invalid_timestamp) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  if (!m_running_flag.load()) {
    // We get here if we were stopped before the TimestampEstimator received any TimeSyncs
    return;
  }

  dfmessages::timestamp_t ts = m_timestamp_estimator->get_timestamp_estimate();

  // This case should have been caught in do_resume()
  assert(m_trigger_interval_ticks.load() != 0);

  TLOG_DEBUG(1) << "Delaying trigger decision sending by " << trigger_delay_ticks_ << " ticks";
  // Round up to the next multiple of trigger_interval_ticks_
  dfmessages::timestamp_t next_trigger_timestamp =
    (ts / m_trigger_interval_ticks.load() + 1) * m_trigger_interval_ticks.load() + m_trigger_offset;
  TLOG_DEBUG(1) << "Initial timestamp estimate is " << ts << ", next_trigger_timestamp is " << next_trigger_timestamp;

  assert(next_trigger_timestamp > ts);

  while (true) {
    while (m_running_flag.load() &&
           (m_timestamp_estimator->get_timestamp_estimate() < (next_trigger_timestamp + trigger_delay_ticks_) ||
            m_timestamp_estimator->get_timestamp_estimate() == dfmessages::TypeDefaults::s_invalid_timestamp)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!m_running_flag.load())
      break;

    auto tokens_available = m_token_source != nullptr ? m_tokens.load() : 1;
    if (!triggers_are_inhibited() && !m_paused.load() && tokens_available > 0) {

      dfmessages::TriggerDecision decision = create_decision(next_trigger_timestamp);

      for (int i = 0; i < m_repeat_trigger_count; ++i) {
        TLOG_DEBUG(1) << "At timestamp " << m_timestamp_estimator->get_timestamp_estimate()
                      << ", pushing a decision with triggernumber " << decision.trigger_number << " timestamp "
                      << decision.trigger_timestamp << " number of links " << decision.components.size();
        dfmessages::TriggerDecision decision_copy(decision);
        m_trigger_decision_sink->send(std::move(decision_copy), std::chrono::milliseconds(10));
        std::lock_guard<std::mutex> lk(m_open_trigger_decisions_mutex);
        m_open_trigger_decisions.insert(decision.trigger_number);
        decision.trigger_number++;
        m_last_trigger_number++;
        m_tokens--;
        m_trigger_count++;
        m_trigger_count_tot++;
      }
    } else if (tokens_available == 0) {
      TLOG_DEBUG(1) << "There are no Tokens available. Not sending a TriggerDecision for timestamp "
                    << next_trigger_timestamp;
      m_inhibited_trigger_count++;
      m_inhibited_trigger_count_tot++;
    } else {
      TLOG_DEBUG(1) << "Triggers are inhibited/paused. Not sending a TriggerDecision for timestamp "
                    << next_trigger_timestamp;
    }

    next_trigger_timestamp += m_trigger_interval_ticks.load();
  }

  // We get here after the stop command is received. We send out
  // m_stop_burst_count triggers in one go, so that there are triggers
  // in-flight in the system during the stopping transition. This is
  // intended to allow tests that all of the queues are correctly
  // drained elsewhere in the system during the stop transition
  if (m_stop_burst_count) {
    TLOG_DEBUG(0) << "Sending " << m_stop_burst_count << " triggers at stop";
    dfmessages::TriggerDecision decision = create_decision(next_trigger_timestamp);

    for (int i = 0; i < m_stop_burst_count; ++i) {
        dfmessages::TriggerDecision decision_copy(decision);
      m_trigger_decision_sink->send(std::move(decision_copy), std::chrono::milliseconds(10));
      decision.trigger_number++;
      m_last_trigger_number++;
      m_trigger_count++;
      m_trigger_count_tot++;
    }
  }
}

void
TriggerDecisionEmulator::read_inhibit_queue()
{
  if (m_trigger_inhibit_source == nullptr)
    return;

  // This loop is a hack to deal with the fact that there might be
  // leftover TriggerInhibit messages from the previous run, because
  // TriggerDecisionEmulator is stopped before the DF modules that
  // send the TriggerInhibits. So we pop everything we can at
  // startup. This will definitely get all of the TriggerInhibits from
  // the previous run. It *may* also get TriggerInhibits from the
  // current run, which we drop on the floor. This is not really
  // ideal, since we don't know whether a given message on the queue
  // came from the previous (and should be ignored) or from this run
  // (and should be handled). The problem would be when an inhibit is
  // sent in this run before this loop gets started. That seems
  // unlikely, and the whole way we do inhibits is changing for
  // MiniDAQApp v2 anyway, so I'm leaving it like this
  try {
    while (true) {
      dfmessages::TriggerInhibit ti;
      ti = m_trigger_inhibit_source->receive(std::chrono::milliseconds(1));
    }
  } catch (iomanager::TimeoutExpired&) {
  }
  while (m_running_flag.load()) {
    try {
      while (true) {
        dfmessages::TriggerInhibit ti;
        ti = m_trigger_inhibit_source->receive(std::chrono::milliseconds(1));
        m_inhibited.store(ti.busy);
        if (ti.busy) {
          TLOG() << "Dataflow is BUSY.";
        }
      }
    } catch (iomanager::TimeoutExpired&) {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void
TriggerDecisionEmulator::read_token_queue()
{
  if (m_token_source == nullptr)
    return;

  auto open_trigger_report_time = std::chrono::steady_clock::now();
  while (m_running_flag.load()) {
    try {

      while (true) {
        dfmessages::TriggerDecisionToken tdt = m_token_source->receive(std::chrono::milliseconds(1));
        TLOG_DEBUG(1) << "Received token with run number " << tdt.run_number << ", current run number " << m_run_number;
        if (tdt.run_number == m_run_number) {
          m_tokens++;
          TLOG_DEBUG(1) << "There are now " << m_tokens.load() << " tokens available";

          if (tdt.trigger_number != dfmessages::TypeDefaults::s_invalid_trigger_number) {
            if (m_open_trigger_decisions.count(tdt.trigger_number)) {
              std::lock_guard<std::mutex> lk(m_open_trigger_decisions_mutex);
              m_open_trigger_decisions.erase(tdt.trigger_number);
              TLOG_DEBUG(1) << "Token indicates that trigger decision " << tdt.trigger_number
                            << " has been completed. There are now " << m_open_trigger_decisions.size()
                            << " triggers in flight";
            } else {
              // ERS warning: received token for trigger number I don't recognize
            }
          }
        }
      }
    } catch (iomanager::TimeoutExpired&) {
    }
    if (!m_paused && !m_open_trigger_decisions.empty()) {

      auto now = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<std::chrono::milliseconds>(now - open_trigger_report_time) >
          std::chrono::milliseconds(3000)) {
        std::ostringstream o;
        o << "Open Trigger Decisions: [";
        { // Scope for lock_guard
          bool first = true;
          std::lock_guard<std::mutex> lk(m_open_trigger_decisions_mutex);
          for (auto& td : m_open_trigger_decisions) {
            if (!first)
              o << ", ";
            o << td;
            first = false;
          }
          o << "]";
        }
        TLOG_DEBUG(0) << o.str();
        open_trigger_report_time = now;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

} // namespace trigemu
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::TriggerDecisionEmulator)

// Local Variables:
// c-basic-offset: 2
// End:
