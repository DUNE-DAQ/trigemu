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
  void do_start(const nlohmann::json& obj); 
  void do_stop(const nlohmann::json& obj);
  void do_pause(const nlohmann::json& obj);
  void do_resume(const nlohmann::json& obj);
};
} // namespace trigemu
} // namespace dunedaq

#endif // TRIGEMU_SRC_TRIGGERDECISIONEMULATOR_HPP_

