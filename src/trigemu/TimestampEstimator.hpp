/**
 * @file TimestampEstimator.hpp TimestampEstimator Class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGEMU_SRC_TRIGEMU_TIMESTAMPESTIMATOR_HPP_
#define TRIGEMU_SRC_TRIGEMU_TIMESTAMPESTIMATOR_HPP_

#include "appfwk/DAQSource.hpp"

#include "dfmessages/TimeSync.hpp"
#include "dfmessages/Types.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace dunedaq {
namespace trigemu {

class TimestampEstimator
{
public:
  TimestampEstimator(std::unique_ptr<appfwk::DAQSource<dfmessages::TimeSync>>& time_sync_source,
                     uint64_t clock_frequency_hz); // NOLINT(build/unsigned)

  ~TimestampEstimator();

  TimestampEstimator(TimestampEstimator const&) = delete;
  TimestampEstimator(TimestampEstimator&&) = default;
  TimestampEstimator& operator=(TimestampEstimator const&) = delete;
  TimestampEstimator& operator=(TimestampEstimator&&) = default;

  dfmessages::timestamp_t get_timestamp_estimate() const { return m_current_timestamp_estimate.load(); }

private:
  void estimator_thread_fn(std::unique_ptr<appfwk::DAQSource<dfmessages::TimeSync>>& time_sync_source);

  // The estimate of the current timestamp
  std::atomic<dfmessages::timestamp_t> m_current_timestamp_estimate{ dfmessages::TypeDefaults::s_invalid_timestamp };

  std::atomic<bool> m_running_flag{ false };
  uint64_t m_clock_frequency_hz; // NOLINT
  std::thread m_estimator_thread;
};

} // namespace trigemu
} // namespace dunedaq

#endif // TRIGEMU_SRC_TRIGEMU_TIMESTAMPESTIMATOR_HPP_
