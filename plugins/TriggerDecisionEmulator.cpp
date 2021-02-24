/**
 * @file TriggerDecisionEmulator.cpp TriggerDecisionEmulator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TriggerDecisionEmulator.hpp"


#include "dataformats/ComponentRequest.hpp"

#include "dfmessages/TimeSync.hpp"
#include "dfmessages/TriggerDecision.hpp"
#include "dfmessages/TriggerInhibit.hpp"
#include "dfmessages/Types.hpp"
#include "ers/ers.h"

<<<<<<< HEAD
#include "trigemu/triggerdecisionemulatorinfo/Nljs.hpp"
=======
#include "trigemu/Issues.hpp"
#include "trigemu/TimestampEstimator.hpp"
>>>>>>> develop
#include "trigemu/triggerdecisionemulator/Nljs.hpp"
#include "trigemu/triggerdecisionemulator/Structs.hpp"

#include "appfwk/app/Nljs.hpp"

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
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "time_sync_source") {
      m_time_sync_source.reset(new appfwk::DAQSource<dfmessages::TimeSync>(qi.inst));
    }
    if (qi.name == "trigger_inhibit_source") {
      m_trigger_inhibit_source.reset(new appfwk::DAQSource<dfmessages::TriggerInhibit>(qi.inst));
    }
    if (qi.name == "trigger_decision_sink") {
      m_trigger_decision_sink.reset(new appfwk::DAQSink<dfmessages::TriggerDecision>(qi.inst));
    }
  }
}

void 
TriggerDecisionEmulator::get_info(opmonlib::InfoCollector& ci, int /*level*/) {
  triggerdecisionemulatorinfo::Info tde;
  
  tde.triggers = m_trigger_count_tot.load();
  tde.new_triggers = m_trigger_count.exchange(0);

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

  m_links.clear();
  for (auto const& link : params.links) {
    // For the future: Set APA properly
    m_links.push_back(dfmessages::GeoID{ 0, static_cast<uint32_t>(link) }); // NOLINT
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
  m_run_number = startobj.value<dunedaq::dataformats::run_number_t>("run", 0);

  m_paused.store(true);
  m_inhibited.store(false);
  m_running_flag.store(true);

  m_timestamp_estimator.reset(new TimestampEstimator(m_time_sync_source, m_clock_frequency_hz));
  
  m_read_inhibit_queue_thread=std::thread(&TriggerDecisionEmulator::read_inhibit_queue, this);
  pthread_setname_np(m_read_inhibit_queue_thread.native_handle(), "tde-inhibit-q");

  m_send_trigger_decisions_thread=std::thread(&TriggerDecisionEmulator::send_trigger_decisions, this);
  pthread_setname_np(m_send_trigger_decisions_thread.native_handle(), "tde-trig-dec");
}

void
TriggerDecisionEmulator::do_stop(const nlohmann::json& /*stopobj*/)
{
  m_running_flag.store(false);
  m_timestamp_estimator.reset(nullptr); // Calls TimestampEstimator dtor
  m_read_inhibit_queue_thread.join();
  m_send_trigger_decisions_thread.join();
}

void
TriggerDecisionEmulator::do_pause(const nlohmann::json& /*pauseobj*/)
{
  m_paused.store(true);
  ERS_LOG("******* Triggers PAUSED! *********");
}

void
TriggerDecisionEmulator::do_resume(const nlohmann::json& resumeobj)
{
  auto params = resumeobj.get<triggerdecisionemulator::ResumeParams>();
  m_trigger_interval_ticks.store(params.trigger_interval_ticks);

  ERS_LOG("******* Triggers RESUMED! *********");
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
  decision.m_trigger_number = m_last_trigger_number + 1;
  decision.m_run_number = m_run_number;
  decision.m_trigger_timestamp = timestamp;
  decision.m_trigger_type = m_trigger_type;

  int n_links = n_links_dist(random_engine);

  std::vector<dfmessages::GeoID> this_links;
  std::sample(m_links.begin(), m_links.end(), std::back_inserter(this_links), n_links, random_engine);

  for (auto link : this_links) {
    dfmessages::ComponentRequest request;
    request.m_component = link;
    request.m_window_offset = m_trigger_window_offset;
    request.m_window_width = window_ticks_dist(random_engine);

    decision.m_components.insert({ link, request });
  }

  return decision;
}

void
TriggerDecisionEmulator::send_trigger_decisions()
{

  // We get here at start of run, so reset the trigger number
  m_last_trigger_number=0;
  m_trigger_count.store(0);
  m_trigger_count_tot.store(0);

  // Wait for there to be a valid timestamp estimate before we start
  while (m_running_flag.load() && m_timestamp_estimator->get_timestamp_estimate() == INVALID_TIMESTAMP) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  dfmessages::timestamp_t ts = m_timestamp_estimator->get_timestamp_estimate();
  ERS_DEBUG(1, "Delaying trigger decision sending by " << trigger_delay_ticks_ << " ticks");
  // Round up to the next multiple of trigger_interval_ticks_
  dfmessages::timestamp_t next_trigger_timestamp =
    (ts / m_trigger_interval_ticks.load() + 1) * m_trigger_interval_ticks.load() + m_trigger_offset;
  ERS_DEBUG(1, "Initial timestamp estimate is " << ts << ", next_trigger_timestamp is " << next_trigger_timestamp);

  assert(next_trigger_timestamp > ts);

  while (true) {
    while (m_running_flag.load() &&
           (m_timestamp_estimator->get_timestamp_estimate() < (next_trigger_timestamp + trigger_delay_ticks_) ||
            m_timestamp_estimator->get_timestamp_estimate() == INVALID_TIMESTAMP)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!m_running_flag.load())
      break;

    if (!triggers_are_inhibited() && !m_paused.load()) {

      dfmessages::TriggerDecision decision = create_decision(next_trigger_timestamp);

      for (int i = 0; i < m_repeat_trigger_count; ++i) {
        ERS_DEBUG(0,
                  "At timestamp " << m_timestamp_estimator->get_timestamp_estimate() << ", pushing a decision with triggernumber "
                                  << decision.m_trigger_number << " timestamp " << decision.m_trigger_timestamp
                                  << " number of links " << decision.m_components.size());
        m_trigger_decision_sink->push(decision, std::chrono::milliseconds(10));
        decision.m_trigger_number++;
        m_last_trigger_number++;
        m_trigger_count++;
        m_trigger_count_tot++;
      }
    } else {
      ERS_DEBUG(
        1, "Triggers are inhibited/paused. Not sending a TriggerDecision for timestamp " << next_trigger_timestamp);
    }

    next_trigger_timestamp += m_trigger_interval_ticks.load();
  }

  // We get here after the stop command is received. We send out
  // m_stop_burst_count triggers in one go, so that there are triggers
  // in-flight in the system during the stopping transition. This is
  // intended to allow tests that all of the queues are correctly
  // drained elsewhere in the system during the stop transition
  if(m_stop_burst_count){
    ERS_DEBUG(0, "Sending " << m_stop_burst_count << " triggers at stop");
    dfmessages::TriggerDecision decision = create_decision(next_trigger_timestamp);

    for (int i = 0; i < m_stop_burst_count; ++i) {
      m_trigger_decision_sink->push(decision, std::chrono::milliseconds(10));
      decision.m_trigger_number++;
      m_last_trigger_number++;
      m_trigger_count++;
      m_trigger_count_tot++;

    }
  }

}

void
TriggerDecisionEmulator::read_inhibit_queue()
{
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
  while (m_trigger_inhibit_source->can_pop()) {
    dfmessages::TriggerInhibit ti;
    m_trigger_inhibit_source->pop(ti);
  }
  while (m_running_flag.load()) {
    while (m_trigger_inhibit_source->can_pop()) {
      dfmessages::TriggerInhibit ti;
      m_trigger_inhibit_source->pop(ti);
      m_inhibited.store(ti.m_busy);
      if (ti.m_busy) {
        ERS_LOG("Dataflow is BUSY.");
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
