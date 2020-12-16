#ifndef TRIGEMU_TEST_PLUGINS_FAKETIMESYNCSOURCE_HPP_
#define TRIGEMU_TEST_PLUGINS_FAKETIMESYNCSOURCE_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "dfmessages/TimeSync.hpp"

namespace dunedaq::trigemu {

class FakeTimeSyncSource : public dunedaq::appfwk::DAQModule
{
public:
  explicit FakeTimeSyncSource(const std::string& name);
  
  FakeTimeSyncSource(const FakeTimeSyncSource&) =
    delete; ///< FakeTimeSyncSource is not copy-constructible
  FakeTimeSyncSource& operator=(const FakeTimeSyncSource&) =
    delete; ///< FakeTimeSyncSource is not copy-assignable
  FakeTimeSyncSource(FakeTimeSyncSource&&) =
    delete; ///< FakeTimeSyncSource is not move-constructible
  FakeTimeSyncSource& operator=(FakeTimeSyncSource&&) =
    delete; ///< FakeTimeSyncSource is not move-assignable
  
  void init(const nlohmann::json& iniobj) override;
  
private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  void send_timesyncs(const dfmessages::timestamp_t timesync_interval_ticks);
  
  std::atomic<bool> running_flag_;
  std::vector<std::thread> threads_;
  
  std::unique_ptr<appfwk::DAQSink<dfmessages::TimeSync>> time_sync_sink_;

  dfmessages::timestamp_t sync_interval_ticks_;  
};
  
} // namespace dunedaq::trigemu

#endif // include guard
