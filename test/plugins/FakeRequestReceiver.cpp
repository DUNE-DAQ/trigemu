#include "FakeRequestReceiver.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "dfmessages/TriggerDecision.hpp"

#include "ers/ers.h"

namespace dunedaq::trigemu {

FakeRequestReceiver::FakeRequestReceiver(const std::string& name)
  : DAQModule(name)
  , m_running_flag{ false }
{
  register_command("start", &FakeRequestReceiver::do_start);
  register_command("stop", &FakeRequestReceiver::do_stop);
}

void
FakeRequestReceiver::init(const nlohmann::json& iniobj)
{
  auto ini = iniobj.get<appfwk::cmd::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "trigger_decision_source") {
      m_trigger_decision_source.reset(new appfwk::DAQSource<dfmessages::TriggerDecision>(qi.inst));
    }
  }
}

void
FakeRequestReceiver::do_start(const nlohmann::json& /* startobj */)
{
  m_running_flag.store(true);
  m_threads.emplace_back(&FakeRequestReceiver::run, this);
}

void
FakeRequestReceiver::do_stop(const nlohmann::json& /* stopobj */)
{
  m_running_flag.store(false);
  for (auto& thread : m_threads)
    thread.join();
  m_threads.clear();
}

void
FakeRequestReceiver::run()
{
  int dec_counter = 0;
  while (m_running_flag.load()) {
    if (m_trigger_decision_source->can_pop()) {
      dfmessages::TriggerDecision decision;
      m_trigger_decision_source->pop(decision);
      ++dec_counter;
      if (dec_counter % 10 == 0) {
        ERS_DEBUG(0, "Received " << dec_counter << " trigger decisions.");
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}

} // namespace dunedaq::trigemu

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::FakeRequestReceiver)
