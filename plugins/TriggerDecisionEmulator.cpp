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

#include "trigemu/triggerdecisionemulator/Structs.hpp"
#include "trigemu/triggerdecisionemulator/Nljs.hpp"

#include "appfwk/cmd/Nljs.hpp"

#include <random>
#include <cassert>


namespace dunedaq {
namespace trigemu {



TriggerDecisionEmulator::TriggerDecisionEmulator(const std::string& name)
  : DAQModule(name)
  , run_number_(0)
  , time_sync_source_(nullptr)
  , trigger_inhibit_source_(nullptr)
  , trigger_decision_sink_(nullptr)
  , inhibited_(false)
  , last_trigger_number_(0)
{
  register_command("conf", &TriggerDecisionEmulator::do_configure);
  register_command("start", &TriggerDecisionEmulator::do_start);
  register_command("stop", &TriggerDecisionEmulator::do_stop);
  register_command("pause", &TriggerDecisionEmulator::do_pause);
  register_command("resume", &TriggerDecisionEmulator::do_resume);
}

void
TriggerDecisionEmulator::init(const nlohmann::json& iniobj)
{
  auto ini = iniobj.get<appfwk::cmd::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "time_sync_source") {
      time_sync_source_.reset(new appfwk::DAQSource<dfmessages::TimeSync>(qi.inst));
    }
    if (qi.name == "trigger_inhibit_source") {
      trigger_inhibit_source_.reset(new appfwk::DAQSource<dfmessages::TriggerInhibit>(qi.inst));
    }
    if (qi.name == "trigger_decision_sink") {
      trigger_decision_sink_.reset(new appfwk::DAQSink<dfmessages::TriggerDecision>(qi.inst));
    }
  }
}

void
TriggerDecisionEmulator::do_configure(const nlohmann::json& confobj)
{
  auto params=confobj.get<triggerdecisionemulator::ConfParams>();
  min_readout_window_ticks_=params.min_readout_window_ticks;
  max_readout_window_ticks_=params.max_readout_window_ticks;
  min_links_in_request_=params.min_links_in_request;
  max_links_in_request_=params.max_links_in_request;
  trigger_interval_ticks_.store(params.trigger_interval_ticks);
  clock_frequency_hz_=params.clock_frequency_hz;
  
  links_.clear();
  for(auto const& link: params.links){
    // TODO: Set APA properly
    links_.push_back(dfmessages::GeoID{0, static_cast<uint32_t>(link)});
  }

  // Sanity-check the values
  if(min_readout_window_ticks_ > max_readout_window_ticks_ ||
     min_links_in_request_ > max_links_in_request_){
    throw InvalidConfiguration(ERS_HERE);
  }
}

void
TriggerDecisionEmulator::do_start(const nlohmann::json& startobj)
{
  run_number_ = startobj.value<dunedaq::dataformats::run_number_t>("run", 0);
  current_timestamp_estimate_.store(INVALID_TIMESTAMP);

  running_flag_.store(true);
  paused_.store(false);
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
    paused_.store(true);
    ERS_INFO("******* Triggers PAUSED! *********");
}

void
TriggerDecisionEmulator::do_resume(const nlohmann::json& resumeobj)
{
    auto params=resumeobj.get<triggerdecisionemulator::ResumeParams>();
    trigger_interval_ticks_.store(params.trigger_interval_ticks);

    ERS_INFO("******* Triggers RESUMED! *********");
    paused_.store(false);
}

void TriggerDecisionEmulator::send_trigger_decisions()
{
  // Wait for there to be a valid timestamp estimate before we start
  while(running_flag_.load() &&
        current_timestamp_estimate_.load()==INVALID_TIMESTAMP){
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  dfmessages::timestamp_t ts=current_timestamp_estimate_.load();
  // Round up to the next multiple of trigger_interval_ticks_
  dfmessages::timestamp_t next_trigger_timestamp=(ts/trigger_interval_ticks_.load()   + 1)*trigger_interval_ticks_.load() + trigger_offset_;
  ERS_DEBUG(1,"Initial timestamp estimate is " << ts << ", next_trigger_timestamp is " << next_trigger_timestamp);

  assert(next_trigger_timestamp > ts);

  std::default_random_engine random_engine(run_number_);
  std::uniform_int_distribution<int> n_links_dist(min_links_in_request_,
                                                  std::min((size_t)max_links_in_request_, links_.size()));
  std::uniform_int_distribution<dfmessages::timestamp_t> window_ticks_dist(min_readout_window_ticks_, max_readout_window_ticks_);

  while(true){
    while(running_flag_.load() &&
          (current_timestamp_estimate_.load() < next_trigger_timestamp ||
           current_timestamp_estimate_.load()==INVALID_TIMESTAMP)){
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if(!running_flag_.load()) break;

    if(!triggers_are_inhibited() && !paused_.load()){

      dfmessages::TriggerDecision decision;
      decision.trigger_number=last_trigger_number_++;
      decision.run_number=run_number_;
      decision.trigger_timestamp=next_trigger_timestamp;
      decision.trigger_type=trigger_type_;

      int n_links = n_links_dist(random_engine);

      std::vector<dfmessages::GeoID> this_links;
      std::sample(links_.begin(), links_.end(), std::back_inserter(this_links),
                  n_links, random_engine);

      for(auto link: this_links){
        dfmessages::ComponentRequest request;
        request.window_offset=trigger_window_offset_;
        request.window_width=window_ticks_dist(random_engine);

        decision.components.insert({link, request});
      }

      ERS_DEBUG(0,"At timestamp " << current_timestamp_estimate_.load() << ", pushing a decision with triggernumber " << decision.trigger_number
               << " timestamp " << decision.trigger_timestamp
               << " number of links " << n_links);
      trigger_decision_sink_->push(decision, std::chrono::milliseconds(10));
    }
    else{
      ERS_DEBUG(1,"Triggers are inhibited. Not sending a TriggerDecision for timestamp " << next_trigger_timestamp);
    }

    next_trigger_timestamp+=trigger_interval_ticks_.load();
  }

}

void TriggerDecisionEmulator::estimate_current_timestamp()
{
  dfmessages::TimeSync most_recent_timesync{INVALID_TIMESTAMP};
  current_timestamp_estimate_.store(INVALID_TIMESTAMP);

  int i=0;

  // time_sync_source_ is connected to an MPMC queue with multiple
  // writers. We read whatever we can off it, and the item with the
  // largest timestamp "wins"
  while(running_flag_.load()){
    // First, update the latest timestamp
    while(time_sync_source_->can_pop()){
      dfmessages::TimeSync t{INVALID_TIMESTAMP};
      time_sync_source_->pop(t);
      ERS_DEBUG(1,"Got a TimeSync timestamp = " << t.DAQ_time << ", system time = " << t.system_time);
      if(most_recent_timesync.DAQ_time==INVALID_TIMESTAMP ||
         t.DAQ_time > most_recent_timesync.DAQ_time){
        most_recent_timesync=t;
      }
    }

    if(most_recent_timesync.DAQ_time!=INVALID_TIMESTAMP){
      // Update the current timestamp estimate, based on the most recently-read TimeSync
      using namespace std::chrono;
      // std::chrono is the worst
      auto time_now=static_cast<uint64_t>(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());
      if(time_now < most_recent_timesync.system_time){
        ers::error(InvalidTimeSync(ERS_HERE));
      }
      else {
          auto delta_time=time_now - most_recent_timesync.system_time;
          const dfmessages::timestamp_t new_timestamp=most_recent_timesync.DAQ_time + delta_time*clock_frequency_hz_/1000000;
          if(i++ % 100 == 0){
              ERS_DEBUG(1,"Updating timestamp estimate to " << new_timestamp);
          }
          current_timestamp_estimate_.store(new_timestamp);
      }
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
      inhibited_.store(ti.busy);
      if(ti.busy) {
	ERS_INFO("Dataflow is BUSY.");
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
