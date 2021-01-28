/**
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGEMU_TEST_PLUGINS_FAKEREQUESTRECEIVER_HPP_
#define TRIGEMU_TEST_PLUGINS_FAKEREQUESTRECEIVER_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "dfmessages/TriggerDecision.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq::trigemu {

class FakeRequestReceiver : public dunedaq::appfwk::DAQModule
{
public:
  explicit FakeRequestReceiver(const std::string& name);

  FakeRequestReceiver(const FakeRequestReceiver&) = delete; ///< FakeRequestReceiver is not copy-constructible
  FakeRequestReceiver& operator=(const FakeRequestReceiver&) = delete; ///< FakeRequestReceiver is not copy-assignable
  FakeRequestReceiver(FakeRequestReceiver&&) = delete;            ///< FakeRequestReceiver is not move-constructible
  FakeRequestReceiver& operator=(FakeRequestReceiver&&) = delete; ///< FakeRequestReceiver is not move-assignable

  void init(const nlohmann::json& iniobj) override;

private:
  // Data Source
  std::unique_ptr<appfwk::DAQSource<dfmessages::TriggerDecision>> m_trigger_decision_source;
  // Commands
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  void run();

  std::atomic<bool> m_running_flag;
  std::vector<std::thread> m_threads;
};

} // namespace dunedaq::trigemu

#endif // TRIGEMU_TEST_PLUGINS_FAKEREQUESTRECEIVER_HPP_
