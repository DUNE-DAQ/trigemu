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

// TODO: Point this to the real header containing message class
// definitions once it becomes available
#include "dfmessages/TimeSync.hpp"
#include "dfmessages/TriggerDecision.hpp"
#include "dfmessages/TriggerInhibit.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include <ers/Issue.h>

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
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

  void do_work(std::atomic<bool>& running_flag);

  // Estimate what the current timestamp is, based on what we've seen
  // in the TimeSync queue
  dfmessages::timestamp_t estimate_current_timestamp();

  // Wait until our estimate of the current timestamp reaches
  // `timestamp`, or `running_flag` is set to false, whichever occurs
  // first. Return true if timestamp is reached, false if
  // we returned because `running_flag` was set to false
  bool wait_until_timestamp(dfmessages::timestamp_t timestamp, std::atomic<bool>& running_flag);

  // Are we inhibited from sending triggers?
  bool triggers_are_inhibited();

  // Send a trigger decision
  void send_trigger_decision(dfmessages::TriggerDecision decision);
  
  dunedaq::appfwk::ThreadHelper thread_;

  // Queue sources and sinks
  std::vector<std::unique_ptr<appfwk::DAQSource<dfmessages::TimeSync>>> time_sync_sources_;
  std::unique_ptr<appfwk::DAQSource<dfmessages::TriggerInhibit>> trigger_inhibit_source_;
  std::unique_ptr<appfwk::DAQSink<dfmessages::TriggerDecision>> trigger_decision_sink_;

  // Variables controlling how we produce triggers

  // Triggers are produced for timestamps:
  //    timestamp_offset_ + n*timestamp_period_;
  // with n integer
  dfmessages::timestamp_t timestamp_offset_;
  dfmessages::timestamp_t timestamp_period_;

  dfmessages::timestamp_diff_t trigger_window_offset_;
  dfmessages::timestamp_t trigger_window_width_;
  
  // The link IDs which should be read out in the trigger decision
  std::vector<int> active_link_ids_;

  // If false, all links are read at each trigger. If true, we read
  // out just one link for each trigger, cycling through
  // active_link_ids_
  bool cycle_through_links_;

  // The most recent TimeSync message we've seen
  dfmessages::TimeSync most_recent_timesync_;

  // The most recent inhibit status we've seen (true = inhibited)
  bool inhibited_;
};
} // namespace trigemu
} // namespace dunedaq

#endif // TRIGEMU_SRC_TRIGGERDECISIONEMULATOR_HPP_


// Local Variables:
// c-basic-offset: 2
// End:
