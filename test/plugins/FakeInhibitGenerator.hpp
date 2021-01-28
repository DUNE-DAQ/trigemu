#ifndef TRIGEMU_TEST_PLUGINS_FAKEINHIBITGENERATOR_HPP_
#define TRIGEMU_TEST_PLUGINS_FAKEINHIBITGENERATOR_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"

#include "dfmessages/TriggerInhibit.hpp"

namespace dunedaq::trigemu {

class FakeInhibitGenerator : public dunedaq::appfwk::DAQModule
{
public:
  explicit FakeInhibitGenerator(const std::string& name);

  FakeInhibitGenerator(const FakeInhibitGenerator&) = delete; ///< FakeInhibitGenerator is not copy-constructible
  FakeInhibitGenerator& operator=(const FakeInhibitGenerator&) =
    delete;                                                         ///< FakeInhibitGenerator is not copy-assignable
  FakeInhibitGenerator(FakeInhibitGenerator&&) = delete;            ///< FakeInhibitGenerator is not move-constructible
  FakeInhibitGenerator& operator=(FakeInhibitGenerator&&) = delete; ///< FakeInhibitGenerator is not move-assignable

  void init(const nlohmann::json& iniobj) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  void send_inhibits(const std::chrono::milliseconds inhibit_interval_ms);

  std::atomic<bool> m_running_flag;
  std::vector<std::thread> m_threads;

  std::unique_ptr<appfwk::DAQSink<dfmessages::TriggerInhibit>> m_trigger_inhibit_sink;
  std::chrono::milliseconds m_inhibit_interval_ms;
};

} // namespace dunedaq::trigemu

#endif // include guard
