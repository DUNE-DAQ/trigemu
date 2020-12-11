/**
 * @file TriggerDecisionEmulator.cpp TriggerDecisionEmulator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TriggerDecisionEmulator.hpp"
#include "dfmessages/ComponentRequest.hpp"
#include "dfmessages/Types.hpp"
#include "ers/ers.h"
#include "trigemu/Messages_dummy.hpp"


namespace dunedaq {
namespace trigemu {

constexpr dfmessages::timestamp_t INVALID_TIMESTAMP=0xffffffffffffffff;

TriggerDecisionEmulator::TriggerDecisionEmulator(const std::string& name)
  : DAQModule(name)
  , thread_(std::bind(&TriggerDecisionEmulator::do_work, this, std::placeholders::_1))
  , most_recent_timesync_(INVALID_TIMESTAMP)
  , inhibited_(false)
  , last_trigger_number_(0)
{
  register_command("configure", &TriggerDecisionEmulator::do_configure);
  register_command("start", &TriggerDecisionEmulator::do_start);
  register_command("stop", &TriggerDecisionEmulator::do_stop);
  register_command("pause", &TriggerDecisionEmulator::do_pause);
  register_command("resume", &TriggerDecisionEmulator::do_resume);
}

void
TriggerDecisionEmulator::init(const nlohmann::json& /* iniobj */)
{
}

void
TriggerDecisionEmulator::do_configure(const nlohmann::json& /* confobj */)
{
  // TODO:
  // Fill the following from config:
  // * time_sync_sources_
  // * trigger_inhibit_source_
  // * trigger_decision_sink_
  // * active_link_ids_
  // * timestamp_offset_
  // * timestamp_period_
  // * cycle_through_links_
  // * trigger_window_offset_
  // * trigger_window_width_
  // * trigger_type_
  //
  // ...alternatively, we could just hold a copy of the Conf object as a member variable
}

void
TriggerDecisionEmulator::do_start(const nlohmann::json& /*startobj*/)
{
  // TODO: Set run_number_
  thread_.start_working_thread();
}

void
TriggerDecisionEmulator::do_stop(const nlohmann::json& /*stopobj*/)
{
  thread_.stop_working_thread();
}

void
TriggerDecisionEmulator::do_pause(const nlohmann::json& /*pauseobj*/)
{
}

void
TriggerDecisionEmulator::do_resume(const nlohmann::json& /*resumeobj*/)
{
}

void
TriggerDecisionEmulator::do_work(std::atomic<bool>& running_flag)
{
  dfmessages::timestamp_t current_timestamp=estimate_current_timestamp();
  dfmessages::timestamp_t next_trigger_timestamp=(current_timestamp/timestamp_period_+1)*timestamp_period_+timestamp_offset_;
  size_t last_triggered_link=0;

  while(running_flag.load()){
    bool wait_success=wait_until_timestamp(next_trigger_timestamp, running_flag);
    if(!wait_success) break;
    if(!triggers_are_inhibited()){
      dfmessages::TriggerDecision decision;
      decision.TriggerNumber=last_trigger_number_++;
      decision.RunNumber=run_number_;
      decision.TriggerTimestamp=next_trigger_timestamp;
      decision.TriggerType=trigger_type_;
      if(cycle_through_links_){
        size_t next_link=(last_triggered_link+1)%active_link_ids_.size();
        decision.Components.insert({active_link_ids_[next_link],
            dfmessages::ComponentRequest{next_trigger_timestamp,
                                      trigger_window_offset_,
                                      trigger_window_width_}});
        last_triggered_link=next_link;

      }
      else{
        for(auto const& link: active_link_ids_){
          decision.Components.insert({link,
              dfmessages::ComponentRequest{next_trigger_timestamp,
                                        trigger_window_offset_,
                                        trigger_window_width_}});

        }
      }
      send_trigger_decision(decision);
    }
    next_trigger_timestamp+=timestamp_period_;
  }
}

bool
TriggerDecisionEmulator::wait_until_timestamp(dfmessages::timestamp_t target_timestamp, std::atomic<bool>& running_flag)
{
  while(true){
    try{
      if(estimate_current_timestamp()>=target_timestamp) return true;
    }
    catch(NoTimeSyncsReceived const& ex){
      // Warn, but keep waiting
      ers::warning(ex);
    }

    if(!running_flag.load()) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return true;
}


bool
TriggerDecisionEmulator::triggers_are_inhibited()
{
  // If there are more messages on the inhibit queue, set the current
  // inhibit status to that of the most recent message on the
  // queue. If there are no messages, the status remains as it was
  // before
  while(trigger_inhibit_source_->can_pop()){
    dfmessages::TriggerInhibit ti;
    trigger_inhibit_source_->pop(ti);
    inhibited_=ti.Busy;
  }
  return inhibited_;
}


void
TriggerDecisionEmulator::send_trigger_decision(dfmessages::TriggerDecision decision)
{
  trigger_decision_sink_->push(decision);
}


dfmessages::timestamp_t
TriggerDecisionEmulator::estimate_current_timestamp()
{
  std::unique_ptr<appfwk::DAQSource<dfmessages::TimeSync>>& time_sync_source=time_sync_sources_.at(0);
  while(time_sync_source->can_pop()){
    time_sync_source->pop(most_recent_timesync_);
  }

  if(most_recent_timesync_.DAQTime==INVALID_TIMESTAMP){
    throw NoTimeSyncsReceived(ERS_HERE);
  }

  using namespace std::chrono;
  // std::chrono is the worst
  auto time_now=static_cast<uint64_t>(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());
  if(time_now < most_recent_timesync_.SystemTime){
    throw InvalidTimeSync(ERS_HERE);
  }
  auto delta_time=time_now - most_recent_timesync_.SystemTime;
  // TODO: Make the "stale timestamp" time configurable
  if(delta_time > 3000000){
    // We haven't had a time sync message for more than three seconds. Warn
  }
  const uint64_t CLOCK_FREQUENCY_HZ=50000000;
  return most_recent_timesync_.DAQTime + delta_time*CLOCK_FREQUENCY_HZ/1000000;
}

} // namespace trigemu
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::TriggerDecisionEmulator)


// Local Variables:
// c-basic-offset: 2
// End:
