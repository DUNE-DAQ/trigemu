/**
 * @file TriggerDecisionEmulator.hpp
 *
 * TriggerDecisionEmulator is a DAQModule that generates trigger decisions
 * for standalone tests. It receives information on the current time and the
 * availability of the DF to absorb data and forms decisions at a configurable
 * rate and with configurable size.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGEMU_SRC_TRIGGERDECISIONEMULATOR_HPP_
#define TRIGEMU_SRC_TRIGGERDECISIONEMULATOR_HPP_

#include "cdf/GeoID.hpp"
#include "dfmessages/TimeSync.hpp"
#include "dfmessages/TriggerDecision.hpp"
#include "dfmessages/TriggerInhibit.hpp"
#include "dfmessages/Types.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include <ers/ers.h>

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {

ERS_DECLARE_ISSUE(trigemu,
                  InvalidTimeSync,
                  "An invalid TimeSync message was received",
                  ERS_EMPTY)

ERS_DECLARE_ISSUE(trigemu,
                  InvalidConfiguration,
                  "An invalid configuration object was received",
                  ERS_EMPTY)


namespace trigemu {

/**
 * @brief TriggerDecisionEmulator reads lists of integers from one queue,
 * reverses the order of the list, and writes out the reversed list.
 */
class TriggerDecisionEmulator : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief TriggerDecisionEmulator Constructor
   * @param name Instance name for this TriggerDecisionEmulator instance
   */
  explicit TriggerDecisionEmulator(const std::string& name);

  TriggerDecisionEmulator(const TriggerDecisionEmulator&) =
    delete; ///< TriggerDecisionEmulator is not copy-constructible
  TriggerDecisionEmulator& operator=(const TriggerDecisionEmulator&) =
    delete; ///< TriggerDecisionEmulator is not copy-assignable
  TriggerDecisionEmulator(TriggerDecisionEmulator&&) =
    delete; ///< TriggerDecisionEmulator is not move-constructible
  TriggerDecisionEmulator& operator=(TriggerDecisionEmulator&&) =
    delete; ///< TriggerDecisionEmulator is not move-assignable

  void init(const nlohmann::json& iniobj) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_pause(const nlohmann::json& obj);
  void do_resume(const nlohmann::json& obj);

  // Are we inhibited from sending triggers?
  bool triggers_are_inhibited() { return inhibited_.load(); }

  // Thread functions
  void send_trigger_decisions();
  void estimate_current_timestamp();
  void read_inhibit_queue();

  // Queue sources and sinks
  std::unique_ptr<appfwk::DAQSource<dfmessages::TimeSync>> time_sync_source_;
  std::unique_ptr<appfwk::DAQSource<dfmessages::TriggerInhibit>> trigger_inhibit_source_;
  std::unique_ptr<appfwk::DAQSink<dfmessages::TriggerDecision>> trigger_decision_sink_;

  static constexpr dfmessages::timestamp_t INVALID_TIMESTAMP=0xffffffffffffffff;

  // Variables controlling how we produce triggers

  // Triggers are produced for timestamps:
  //    trigger_offset_ + n*trigger_interval_ticks_;
  // with n integer
  dfmessages::timestamp_t trigger_offset_{0};
  std::atomic<dfmessages::timestamp_t> trigger_interval_ticks_{0};

  // The offset and width of the windows to be requested in the trigger
  dfmessages::timestamp_diff_t trigger_window_offset_{0};
  dfmessages::timestamp_t min_readout_window_ticks_{0};
  dfmessages::timestamp_t max_readout_window_ticks_{0};

  // The trigger type for the trigger requests
  dfmessages::trigger_type_t trigger_type_{0xff};

  // The link IDs which should be read out in the trigger decision
  std::vector<dfmessages::GeoID> links_;
  int min_links_in_request_;
  int max_links_in_request_;

  // The estimate of the current timestamp
  std::atomic<dfmessages::timestamp_t> current_timestamp_estimate_{INVALID_TIMESTAMP};


  // The most recent inhibit status we've seen (true = inhibited)
  std::atomic<bool> inhibited_;
  // paused state, equivalent to inhibited
  std::atomic<bool> paused_;

  dfmessages::trigger_number_t last_trigger_number_;

  dfmessages::run_number_t run_number_;

  std::vector<std::thread> threads_;
  std::atomic<bool> running_flag_;
};
} // namespace trigemu
} // namespace dunedaq

#endif // TRIGEMU_SRC_TRIGGERDECISIONEMULATOR_HPP_


// Local Variables:
// c-basic-offset: 2
// End:
