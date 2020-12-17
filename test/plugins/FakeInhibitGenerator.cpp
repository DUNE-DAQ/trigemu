#include "FakeInhibitGenerator.hpp"

#include "trigemu/fakeinhibitgenerator/Nljs.hpp"

#include "dfmessages/TriggerInhibit.hpp"
#include "dfmessages/Types.hpp"

#include "appfwk/cmd/Nljs.hpp"

#include <cstdint>

#include "ers/ers.h"

namespace dunedaq::trigemu {

FakeInhibitGenerator::FakeInhibitGenerator(const std::string& name)
  : DAQModule(name)
  , running_flag_{false}
  , inhibit_interval_ms_{0}
{
  register_command("conf",      &FakeInhibitGenerator::do_configure);
  register_command("start",     &FakeInhibitGenerator::do_start);
  register_command("stop",      &FakeInhibitGenerator::do_stop);
}

void
FakeInhibitGenerator::init(const nlohmann::json& iniobj)
{
  auto ini = iniobj.get<appfwk::cmd::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "trigger_inhibit_sink") {
      trigger_inhibit_sink_.reset(new appfwk::DAQSink<dfmessages::TriggerInhibit>(qi.inst));
    }
  }
}

void
FakeInhibitGenerator::do_configure(const nlohmann::json& confobj)
{
  auto params=confobj.get<fakeinhibitgenerator::ConfParams>();
  inhibit_interval_ms_ = std::chrono::milliseconds(params.inhibit_interval_ms);
}

void
FakeInhibitGenerator::do_start(const nlohmann::json& /*startobj*/)
{
  running_flag_.store(true);
  threads_.push_back(std::thread(&FakeInhibitGenerator::send_inhibits, this, inhibit_interval_ms_));
                                 
}

void
FakeInhibitGenerator::do_stop(const nlohmann::json& /* stopobj */)
{
  running_flag_.store(false);
  for(auto& thread: threads_) thread.join();
  threads_.clear();
}

void
FakeInhibitGenerator::send_inhibits(const std::chrono::milliseconds inhibit_interval_ms)
{
  auto time_now=system_clock::now();
  auto next_switch_time=time_now+inhibit_interval_ms;
  bool busy=false;
  
  while(true){
    while(running_flag_.load() && system_clock::now() < next_switch_time){
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    if(!running_flag_.load()) break;

    busy=!busy;
    ERS_DEBUG(1,"Sending TriggerInhibit with busy=" << busy);
    trigger_inhibit_sink_->push(dfmessages::TriggerInhibit{busy});
    
    next_switch_time += inhibit_interval_ms;
  }
}
  
} // namespace dunedaq::trigemu

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::FakeInhibitGenerator)
