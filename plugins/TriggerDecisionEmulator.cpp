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

#include "dfmessages/GeoID.hpp"
#include "dfmessages/Types.hpp"
#include "ers/ers.h"

#include "trigemu/triggerdecisionemulator/Structs.hpp"
#include "trigemu/triggerdecisionemulator/Nljs.hpp"

#include <random>

namespace dunedaq {
namespace trigemu {

constexpr dfmessages::timestamp_t INVALID_TIMESTAMP=0xffffffffffffffff;

TriggerDecisionEmulator::TriggerDecisionEmulator(const std::string& name)
  : DAQModule(name)
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
TriggerDecisionEmulator::do_configure(const nlohmann::json& confobj)
{
  auto params=confobj.get<triggerdecisionemulator::ConfParams>();
  min_readout_window_ticks_=params.min_readout_window_ticks;
  max_readout_window_ticks_=params.max_readout_window_ticks;
  min_links_in_request_=params.min_links_in_request;
  max_links_in_request_=params.max_links_in_request;
  for(auto const& link: params.links){
    // TODO: Set APA properly
    links_.push_back(dfmessages::GeoID{0, static_cast<uint32_t>(link)});
  }

}

void
TriggerDecisionEmulator::do_start(const nlohmann::json& /*startobj*/)
{
  // TODO: Set run_number_

  running_flag_.store(true);
  threads_.emplace_back(&TriggerDecisionEmulator::estimate_current_timestamp, this);
  threads_.emplace_back(&TriggerDecisionEmulator::read_inhibit_queue, this);
  threads_.emplace_back(&TriggerDecisionEmulator::send_trigger_decisions, this);
}

void
TriggerDecisionEmulator::do_stop(const nlohmann::json& /*stopobj*/)
{
  running_flag_.store(false);
  for(auto& thread: threads_) thread.join();
  threads_.clear();
}

void
TriggerDecisionEmulator::do_pause(const nlohmann::json& /*pauseobj*/)
{
}

void
TriggerDecisionEmulator::do_resume(const nlohmann::json& /*resumeobj*/)
{
}

void TriggerDecisionEmulator::send_trigger_decisions()
{
  dfmessages::timestamp_t next_trigger_timestamp=(current_timestamp_estimate_.load()/trigger_interval_ticks_+1)*trigger_interval_ticks_+trigger_offset_;

  std::default_random_engine random_engine(run_number_);
  std::uniform_int_distribution<int> n_links_dist(min_links_in_request_, max_links_in_request_);
  std::uniform_int_distribution<dfmessages::timestamp_t> window_ticks_dist(min_readout_window_ticks_, max_readout_window_ticks_);

  while(true){
    while(running_flag_.load() &&
          (current_timestamp_estimate_.load() < next_trigger_timestamp ||
           current_timestamp_estimate_.load()==INVALID_TIMESTAMP)){
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if(!running_flag_.load()) break;

    if(!triggers_are_inhibited()){

      dfmessages::TriggerDecision decision;
      decision.TriggerNumber=last_trigger_number_++;
      decision.RunNumber=run_number_;
      decision.TriggerTimestamp=next_trigger_timestamp;
      decision.TriggerType=trigger_type_;

      int n_links = n_links_dist(random_engine);

      std::vector<dfmessages::GeoID> this_links;
      std::sample(links_.begin(), links_.end(), std::back_inserter(this_links),
                  n_links, random_engine);

      for(auto link: this_links){
        dfmessages::ComponentRequest request;
        request.RequestTimestamp=next_trigger_timestamp;
        request.RequestOffset=trigger_window_offset_;
        request.RequestWidth=window_ticks_dist(random_engine);

        decision.Components.insert({link, request});
      }

      trigger_decision_sink_->push(decision);
    }
    next_trigger_timestamp+=trigger_interval_ticks_;
  }

}

void TriggerDecisionEmulator::estimate_current_timestamp()
{
  dfmessages::TimeSync most_recent_timesync{INVALID_TIMESTAMP};
  current_timestamp_estimate_.store(INVALID_TIMESTAMP);

  // time_sync_source_ is connected to an MPMC queue with multiple
  // writers. We read whatever we can off it, and the item with the
  // largest timestamp "wins"
  while(running_flag_.load()){
    // First, update the latest timestamp
    while(time_sync_source_->can_pop()){
      dfmessages::TimeSync t{INVALID_TIMESTAMP};
      time_sync_source_->pop(t);
      if(most_recent_timesync.DAQTime==INVALID_TIMESTAMP ||
         t.DAQTime > most_recent_timesync.DAQTime){
        most_recent_timesync=t;
      }
    }

    if(most_recent_timesync.DAQTime!=INVALID_TIMESTAMP){
      // Update the current timestamp estimate, based on the most recently-read TimeSync
      using namespace std::chrono;
      // std::chrono is the worst
      auto time_now=static_cast<uint64_t>(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());
      if(time_now < most_recent_timesync.SystemTime){
        throw InvalidTimeSync(ERS_HERE);
      }
      auto delta_time=time_now - most_recent_timesync.SystemTime;
      const uint64_t CLOCK_FREQUENCY_HZ=62500000;
      current_timestamp_estimate_.store(most_recent_timesync.DAQTime + delta_time*CLOCK_FREQUENCY_HZ/1000000);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

}

void TriggerDecisionEmulator::read_inhibit_queue()
{
  while(running_flag_.load()){
    while(trigger_inhibit_source_->can_pop()){
      dfmessages::TriggerInhibit ti;
      trigger_inhibit_source_->pop(ti);
      inhibited_.store(ti.Busy);
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
