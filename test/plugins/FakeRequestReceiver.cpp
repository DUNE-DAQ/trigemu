/**
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "FakeRequestReceiver.hpp"
#include "appfwk/app/Nljs.hpp"
#include "dfmessages/TriggerDecision.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "iomanager/IOManager.hpp"

#include "logging/Logging.hpp"

#include <string>

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
  auto ini = iniobj.get<appfwk::app::ModInit>();
  auto qi = appfwk::connection_inst(iniobj, "trigger_decision_source");
  m_trigger_decision_source = get_iom_receiver<dfmessages::TriggerDecision>(qi);
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
    try{
      dfmessages::TriggerDecision decision;
      decision = m_trigger_decision_source->receive(std::chrono::milliseconds(10));
      ++dec_counter;
      if (dec_counter % 10 == 0) {
        TLOG_DEBUG(0) << "Received " << dec_counter << " trigger decisions.";
      }
    } catch (iomanager::TimeoutExpired const&) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}

} // namespace dunedaq::trigemu

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::FakeRequestReceiver)
