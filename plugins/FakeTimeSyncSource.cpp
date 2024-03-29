/**
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "FakeTimeSyncSource.hpp"
#include "appfwk/app/Nljs.hpp"

#include "logging/Logging.hpp"

#include "dfmessages/TimeSync.hpp"
#include "dfmessages/Types.hpp"
#include "iomanager/IOManager.hpp"

#include "trigemu/faketimesyncsource/Nljs.hpp"
#include "trigemu/faketimesyncsource/Structs.hpp"

#include <string>

namespace dunedaq::trigemu {

FakeTimeSyncSource::FakeTimeSyncSource(const std::string& name)
  : DAQModule(name)
  , m_running_flag{ false }
  , m_sync_interval_ticks{ 0 }
  , m_clock_frequency_hz{ 50 * 1000 * 1000 }
{
  register_command("conf", &FakeTimeSyncSource::do_configure);
  register_command("start", &FakeTimeSyncSource::do_start);
  register_command("stop", &FakeTimeSyncSource::do_stop);
}

void
FakeTimeSyncSource::init(const nlohmann::json& iniobj)
{
  auto ini = iniobj.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.conn_refs) {
    if (qi.name == "time_sync_sink") {
      m_time_sync_sink = get_iom_sender<dfmessages::TimeSync>(qi);
    }
  }
}

void
FakeTimeSyncSource::do_configure(const nlohmann::json& confobj)
{
  auto params = confobj.get<faketimesyncsource::ConfParams>();
  m_sync_interval_ticks = params.sync_interval_ticks;
  m_clock_frequency_hz = params.clock_frequency_hz;
}

void
FakeTimeSyncSource::do_start(const nlohmann::json& /*startobj*/)
{
  m_running_flag.store(true);
  m_threads.push_back(std::thread(&FakeTimeSyncSource::send_timesyncs, this, m_sync_interval_ticks));
}

void
FakeTimeSyncSource::do_stop(const nlohmann::json& /* stopobj */)
{
  m_running_flag.store(false);
  for (auto& thread : m_threads)
    thread.join();
  m_threads.clear();
}

void
FakeTimeSyncSource::send_timesyncs(const dfmessages::timestamp_t timesync_interval_ticks)
{

  using namespace std::chrono;

  // std::chrono is the worst
  auto time_now = system_clock::now().time_since_epoch();
  auto now_system_us = duration_cast<microseconds>(time_now).count();
  uint64_t now_timestamp = now_system_us / 1000000 * m_clock_frequency_hz; // NOLINT(build/unsigned)

  dfmessages::timestamp_t next_timestamp = (now_timestamp / timesync_interval_ticks + 1) * timesync_interval_ticks;

  while (true) {
    while (m_running_flag.load() && now_timestamp < next_timestamp) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      time_now = system_clock::now().time_since_epoch();
      now_system_us = duration_cast<microseconds>(time_now).count();
      now_timestamp = now_system_us / 1000000 * m_clock_frequency_hz;
    }
    if (!m_running_flag.load())
      break;
    TLOG_DEBUG(1) << "Sending TimeSync timestamp =" << now_timestamp << ", system time = " << now_system_us;
    dfmessages::TimeSync now(now_timestamp, now_system_us);
    m_time_sync_sink->send(std::move(now), std::chrono::milliseconds(1));

    next_timestamp += timesync_interval_ticks;
  }
}

} // namespace dunedaq::trigemu

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::FakeTimeSyncSource)
