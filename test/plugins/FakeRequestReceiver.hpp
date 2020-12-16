#ifndef TRIGEMU_TEST_PLUGINS_FAKEREQUESTRECEIVER_HPP_
#define TRIGEMU_TEST_PLUGINS_FAKEREQUESTRECEIVER_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "dfmessages/TriggerDecision.hpp"

namespace dunedaq::trigemu {

class FakeRequestReceiver : public dunedaq::appfwk::DAQModule
{
public:
  explicit FakeRequestReceiver(const std::string& name);
  
  FakeRequestReceiver(const FakeRequestReceiver&) =
    delete; ///< FakeRequestReceiver is not copy-constructible
  FakeRequestReceiver& operator=(const FakeRequestReceiver&) =
    delete; ///< FakeRequestReceiver is not copy-assignable
  FakeRequestReceiver(FakeRequestReceiver&&) =
    delete; ///< FakeRequestReceiver is not move-constructible
  FakeRequestReceiver& operator=(FakeRequestReceiver&&) =
    delete; ///< FakeRequestReceiver is not move-assignable
  
  void init(const nlohmann::json& iniobj) override;
  
private:
  // Data Source
  std::unique_ptr<appfwk::DAQSource<dfmessages::TriggerDecision>> trigger_decision_source_;
  // Commands
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  void run();
  
  std::atomic<bool> running_flag_;
  std::vector<std::thread> threads_;
  
};
  
} // namespace dunedaq::trigemu

#endif // TRIGEMU_TEST_PLUGINS_FAKEREQUESTRECEIVER_HPP_
