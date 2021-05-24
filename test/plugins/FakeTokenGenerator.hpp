/**
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGEMU_TEST_PLUGINS_FAKETOKENGENERATOR_HPP_
#define TRIGEMU_TEST_PLUGINS_FAKETOKENGENERATOR_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"

#include "dfmessages/TriggerDecisionToken.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq::trigemu {

class FakeTokenGenerator : public dunedaq::appfwk::DAQModule
{
public:
  explicit FakeTokenGenerator(const std::string& name);

  FakeTokenGenerator(const FakeTokenGenerator&) = delete;            ///< FakeTokenGenerator is not copy-constructible
  FakeTokenGenerator& operator=(const FakeTokenGenerator&) = delete; ///< FakeTokenGenerator is not copy-assignable
  FakeTokenGenerator(FakeTokenGenerator&&) = delete;                 ///< FakeTokenGenerator is not move-constructible
  FakeTokenGenerator& operator=(FakeTokenGenerator&&) = delete;      ///< FakeTokenGenerator is not move-assignable

  void init(const nlohmann::json& iniobj) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  void send_tokens();

  std::atomic<bool> m_running_flag;
  dfmessages::run_number_t m_run_number;
  std::thread m_token_thread;

  std::unique_ptr<appfwk::DAQSink<dfmessages::TriggerDecisionToken>> m_token_sink;
  int m_initial_tokens;
  int m_token_interval_mean_ms;
  int m_token_interval_sigma_ms;
};

} // namespace dunedaq::trigemu

#endif // TRIGEMU_TEST_PLUGINS_FAKETOKENGENERATOR_HPP_
