/**
 * @file TriggerDecisionEmulator.cpp TriggerDecisionEmulator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TriggerDecisionEmulator.hpp"
#include "ers/ers.h"
#include "trigemu/Messages_dummy.hpp"


namespace dunedaq {
namespace trigemu {

TriggerDecisionEmulator::TriggerDecisionEmulator(const std::string& name)
  : DAQModule(name)
  , thread_(std::bind(&TriggerDecisionEmulator::do_work, this, std::placeholders::_1))
  , inhibited_(false)
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
}

void
TriggerDecisionEmulator::do_start(const nlohmann::json& /*startobj*/)
{
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
  df::timestamp_t current_timestamp=estimate_current_timestamp();
  df::timestamp_t next_trigger_timestamp=(current_timestamp/timestamp_period_+1)*timestamp_period_+timestamp_offset_;

  while(running_flag.load()){
    bool wait_success=wait_until_timestamp(next_trigger_timestamp, running_flag);
    if(!wait_success) break;
    if(!triggers_are_inhibited()){
      send_trigger_decision(df::TriggerDecision{});
    }
    next_trigger_timestamp+=timestamp_period_;
  }
}

bool
TriggerDecisionEmulator::wait_until_timestamp(df::timestamp_t target_timestamp, std::atomic<bool>& running_flag)
{
  while(estimate_current_timestamp()<target_timestamp){
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
    df::TriggerInhibit ti;
    trigger_inhibit_source_->pop(ti);
    inhibited_=ti.busy();
  }
  return inhibited_;
}
  

void
TriggerDecisionEmulator::send_trigger_decision(df::TriggerDecision /* decision */)
{

}
  

df::timestamp_t
TriggerDecisionEmulator::estimate_current_timestamp()
{
  std::unique_ptr<appfwk::DAQSource<df::TimeSync>>& time_sync_source=time_sync_sources_.at(0);
  while(time_sync_source->can_pop()){
    time_sync_source->pop(most_recent_timesync_);
  }
  // TODO: Deal with the case where we haven't received a time sync message yet
  using namespace std::chrono;
  // std::chrono is the worst
  auto time_now=static_cast<uint64_t>(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());
  if(time_now < most_recent_timesync_.system_time()){
    // Something's wrong: time has gone backwards. Throw an appropriate error
  }
  auto delta_time=time_now - most_recent_timesync_.system_time();
  // TODO: Make the "stale timestamp" time configurable
  if(delta_time > 3000000){
    // We haven't had a time sync message for more than three seconds. Warn
  }
  const uint64_t CLOCK_FREQUENCY_HZ=50000000;
  return most_recent_timesync_.timestamp() + delta_time*CLOCK_FREQUENCY_HZ/1000000;
}
  
} // namespace trigemu
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::TriggerDecisionEmulator)


// Local Variables:
// c-basic-offset: 2
// End:
