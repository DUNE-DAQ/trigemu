/**
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "FakeTokenGenerator.hpp"

#include "trigemu/faketokengenerator/Nljs.hpp"

#include "dfmessages/TriggerInhibit.hpp"
#include "dfmessages/Types.hpp"
#include "iomanager/IOManager.hpp"

#include "appfwk/app/Nljs.hpp"

#include "logging/Logging.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <utility>

namespace dunedaq::trigemu {

FakeTokenGenerator::FakeTokenGenerator(const std::string& name)
  : DAQModule(name)
  , m_running_flag{ false }
  , m_token_interval_mean_ms{ 0 }
  , m_token_interval_sigma_ms{ 0 }
{
  register_command("conf", &FakeTokenGenerator::do_configure);
  register_command("start", &FakeTokenGenerator::do_start);
  register_command("stop", &FakeTokenGenerator::do_stop);
}

void
FakeTokenGenerator::init(const nlohmann::json& iniobj)
{
  auto ini = iniobj.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.conn_refs) {
    if (qi.name == "token_sink") {
      m_token_sink = get_iom_sender<dfmessages::TriggerDecisionToken>(qi);
    }
  }
}

void
FakeTokenGenerator::do_configure(const nlohmann::json& confobj)
{
  auto params = confobj.get<faketokengenerator::ConfParams>();
  m_token_interval_mean_ms = params.token_interval_ms;
  m_token_interval_sigma_ms = params.token_sigma_ms;
  m_initial_tokens = params.initial_tokens;
}

void
FakeTokenGenerator::do_start(const nlohmann::json& startobj)
{
  m_run_number = startobj.value<dunedaq::daqdataformats::run_number_t>("run", 0);
  m_running_flag.store(true);
  m_token_thread = std::thread(&FakeTokenGenerator::send_tokens, this);
  pthread_setname_np(m_token_thread.native_handle(), "ftg-token-gen");
}

void
FakeTokenGenerator::do_stop(const nlohmann::json& /* stopobj */)
{
  m_running_flag.store(false);
  m_token_thread.join();
}

void
FakeTokenGenerator::send_tokens()
{
  std::normal_distribution<double> distn(m_token_interval_mean_ms, m_token_interval_sigma_ms);
  std::mt19937 random_engine;

  for (int ti = 0; ti < m_initial_tokens; ++ti) {
    dfmessages::TriggerDecisionToken token;
    token.run_number = m_run_number;
    TLOG_DEBUG(0) << "Pushing initial token with run number " << m_run_number << " onto queue";
    m_token_sink->send(std::move(token), iomanager::Sender::s_block);
  }

  while (m_running_flag.load()) {
    dfmessages::TriggerDecisionToken token;
    token.run_number = m_run_number;
    TLOG_DEBUG(0) << "Pushing token with run number " << m_run_number << " onto queue";
    m_token_sink->send(std::move(token), iomanager::Sender::s_block);
    int interval = static_cast<int>(std::round(distn(random_engine)));
    if (interval <= 0)
      interval = 1;

    TLOG_DEBUG(0) << "Sleeping for " << interval << " ms.";
    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
  }
}

} // namespace dunedaq::trigemu

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::FakeTokenGenerator)
