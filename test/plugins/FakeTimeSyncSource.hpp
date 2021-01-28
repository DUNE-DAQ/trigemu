/**
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGEMU_TEST_PLUGINS_FAKETIMESYNCSOURCE_HPP_
#define TRIGEMU_TEST_PLUGINS_FAKETIMESYNCSOURCE_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "dfmessages/TimeSync.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq::trigemu {

class FakeTimeSyncSource : public dunedaq::appfwk::DAQModule
{
public:
  explicit FakeTimeSyncSource(const std::string& name);

  FakeTimeSyncSource(const FakeTimeSyncSource&) = delete;            ///< FakeTimeSyncSource is not copy-constructible
  FakeTimeSyncSource& operator=(const FakeTimeSyncSource&) = delete; ///< FakeTimeSyncSource is not copy-assignable
  FakeTimeSyncSource(FakeTimeSyncSource&&) = delete;                 ///< FakeTimeSyncSource is not move-constructible
  FakeTimeSyncSource& operator=(FakeTimeSyncSource&&) = delete;      ///< FakeTimeSyncSource is not move-assignable

  void init(const nlohmann::json& iniobj) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  void send_timesyncs(const dfmessages::timestamp_t timesync_interval_ticks);

  std::atomic<bool> m_running_flag;
  std::vector<std::thread> m_threads;

  std::unique_ptr<appfwk::DAQSink<dfmessages::TimeSync>> m_time_sync_sink;

  dfmessages::timestamp_t m_sync_interval_ticks;
};

} // namespace dunedaq::trigemu

#endif // TRIGEMU_TEST_PLUGINS_FAKETIMESYNCSOURCE_HPP_
