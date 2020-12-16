#include "FakeRequestReceiver.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "dfmessages/TriggerDecision.hpp"

#include "ers/ers.h"

namespace dunedaq::trigemu {

FakeRequestReceiver::FakeRequestReceiver(const std::string& name)
  : DAQModule(name)
  , running_flag_{false}
{
  register_command("start",     &FakeRequestReceiver::do_start);
  register_command("stop",      &FakeRequestReceiver::do_stop);
}

void
FakeRequestReceiver::init(const nlohmann::json& iniobj)
{
  auto ini = iniobj.get<appfwk::cmd::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "trigger_decision_source") {
      trigger_decision_source_.reset(new appfwk::DAQSource<dfmessages::TriggerDecision>(qi.inst));
    }
  }
}


void
FakeRequestReceiver::do_start(const nlohmann::json& /* startobj */)
{
  running_flag_.store(true);
  threads_.emplace_back(&FakeRequestReceiver::run, this);
}

void
FakeRequestReceiver::do_stop(const nlohmann::json& /* stopobj */)
{
  running_flag_.store(false);
  for(auto& thread: threads_) thread.join();
  threads_.clear();
}

void
FakeRequestReceiver::run()
{
    int dec_counter=0;
    while(running_flag_.load()){
        if(trigger_decision_source_->can_pop()){
            dfmessages::TriggerDecision decision;
            trigger_decision_source_->pop(decision);
	    ++dec_counter;
	    if(dec_counter%10 == 0) {
		ERS_LOG("Received " << dec_counter << " trigger decisions.");
	    }
        }
        else { 
	    std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
    }
}
  
} // namespace dunedaq::trigemu

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::FakeRequestReceiver)
