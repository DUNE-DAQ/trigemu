/**
 * @file TriggerDecisionEmulator.cpp TriggerDecisionEmulator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TriggerDecisionEmulator.hpp"
#include "ers/ers.h"


namespace dunedaq {
namespace trigemu {

TriggerDecisionEmulator::TriggerDecisionEmulator(const std::string& name)
  : DAQModule(name)
{
  register_command("start", &TriggerDecisionEmulator::do_start);
  register_command("stop", &TriggerDecisionEmulator::do_stop);
  register_command("pause", &TriggerDecisionEmulator::do_pause);
  register_command("resume", &TriggerDecisionEmulator::do_resume);
}

void
TriggerDecisionEmulator::init(const nlohmann::json& iniobj)
{
}

void
TriggerDecisionEmulator::do_start(const nlohmann::json& /*startobj*/)
{
}

void
TriggerDecisionEmulator::do_stop(const nlohmann::json& /*stopobj*/)
{
}


} // namespace trigemu
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::TriggerDecisionEmulator)
