/**
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "FakeInhibitGenerator.hpp"

#include "trigemu/fakeinhibitgenerator/Nljs.hpp"

#include "dfmessages/TriggerInhibit.hpp"
#include "dfmessages/Types.hpp"

#include "appfwk/app/Nljs.hpp"

#include "logging/Logging.hpp"

#include <cstdint>
#include <string>

namespace dunedaq::trigemu {

FakeInhibitGenerator::FakeInhibitGenerator(const std::string& name)
  : DAQModule(name)
  , m_running_flag{ false }
  , m_inhibit_interval_ms{ 0 }
{
  register_command("conf", &FakeInhibitGenerator::do_configure);
  register_command("start", &FakeInhibitGenerator::do_start);
  register_command("stop", &FakeInhibitGenerator::do_stop);
}

void
FakeInhibitGenerator::init(const nlohmann::json& iniobj)
{
  auto ini = iniobj.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "trigger_inhibit_sink") {
      m_trigger_inhibit_sink.reset(new appfwk::DAQSink<dfmessages::TriggerInhibit>(qi.inst));
    }
  }
}

void
FakeInhibitGenerator::do_configure(const nlohmann::json& confobj)
{
  auto params = confobj.get<fakeinhibitgenerator::ConfParams>();
  m_inhibit_interval_ms = std::chrono::milliseconds(params.inhibit_interval_ms);
}

void
FakeInhibitGenerator::do_start(const nlohmann::json& /*startobj*/)
{
  m_running_flag.store(true);
  m_threads.push_back(std::thread(&FakeInhibitGenerator::send_inhibits, this, m_inhibit_interval_ms));
}

void
FakeInhibitGenerator::do_stop(const nlohmann::json& /* stopobj */)
{
  m_running_flag.store(false);
  for (auto& thread : m_threads)
    thread.join();
  m_threads.clear();
}

void
FakeInhibitGenerator::send_inhibits(const std::chrono::milliseconds inhibit_interval_ms)
{
  auto time_now = system_clock::now();
  auto next_switch_time = time_now + inhibit_interval_ms;
  bool busy = false;

  while (true) {
    while (m_running_flag.load() && system_clock::now() < next_switch_time) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (!m_running_flag.load())
      break;

    busy = !busy;
    TLOG_DEBUG(1) << "Sending TriggerInhibit with busy=" << busy;
    m_trigger_inhibit_sink->push(dfmessages::TriggerInhibit{ busy });

    next_switch_time += inhibit_interval_ms;
  }
}

} // namespace dunedaq::trigemu

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::FakeInhibitGenerator)
