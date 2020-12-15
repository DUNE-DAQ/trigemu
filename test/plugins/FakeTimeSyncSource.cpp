#include "FakeTimeSyncSource.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "dfmessages/TimeSync.hpp"
#include "dfmessages/Types.hpp"
#include <cstdint>

#include "ers/ers.h"

namespace dunedaq::trigemu {

FakeTimeSyncSource::FakeTimeSyncSource(const std::string& name)
  : DAQModule(name)
  , running_flag_{false}
{
  register_command("configure", &FakeTimeSyncSource::do_configure);
  register_command("start",     &FakeTimeSyncSource::do_start);
  register_command("stop",      &FakeTimeSyncSource::do_stop);
}

void
FakeTimeSyncSource::init(const nlohmann::json& iniobj)
{
  auto ini = iniobj.get<appfwk::cmd::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "time_sync_sink") {
      time_sync_sink_.reset(new appfwk::DAQSink<dfmessages::TimeSync>(qi.inst));
    }
  }
}

void
FakeTimeSyncSource::do_configure(const nlohmann::json& confobj)
{
}

void
FakeTimeSyncSource::do_start(const nlohmann::json& startobj)
{
  running_flag_.store(true);
  threads_.push_back(std::thread(&FakeTimeSyncSource::send_timesyncs, this));
}

void
FakeTimeSyncSource::do_stop(const nlohmann::json& stopobj)
{
  running_flag_.store(false);
  for(auto& thread: threads_) thread.join();
  threads_.clear();
}

void
FakeTimeSyncSource::send_timesyncs()
{
  const uint64_t CLOCK_FREQUENCY_HZ=62500000;
  // Send message once per second
  const dfmessages::timestamp_t timesync_interval_ticks=CLOCK_FREQUENCY_HZ;
  
  using namespace std::chrono;

  using ticks = duration<uint64_t, std::ratio<1, CLOCK_FREQUENCY_HZ>>;
  
  // std::chrono is the worst
  auto time_now=system_clock::now().time_since_epoch();
  auto now_system_us=duration_cast<microseconds>(time_now).count();
  uint64_t now_timestamp=duration_cast<ticks>(time_now).count();

  dfmessages::timestamp_t next_timestamp=(now_timestamp/timesync_interval_ticks+1)*timesync_interval_ticks;

  while(true){
    while(running_flag_.load() && now_timestamp < next_timestamp){
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      time_now=system_clock::now().time_since_epoch();
      now_system_us=duration_cast<microseconds>(time_now).count();
      now_timestamp=duration_cast<ticks>(time_now).count();
    }
    if(!running_flag_.load()) break;
    ERS_INFO("Sending TimeSync " << now_timestamp << " " << now_system_us);
    time_sync_sink_->push(dfmessages::TimeSync(now_timestamp, now_system_us));
  }
}
  
} // namespace dunedaq::trigemu
