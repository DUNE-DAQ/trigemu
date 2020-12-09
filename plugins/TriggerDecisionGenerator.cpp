/**
 * @file TriggerDecisionGenerator.cpp TriggerDecisionGenerator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TriggerDecisionGenerator.hpp"
#include "ers/ers.h"


namespace dunedaq {
namespace trigemu {

TriggerDecisionGenerator::TriggerDecisionGenerator(const std::string& name)
  : DAQModule(name)
{
  register_command("start", &TriggerDecisionGenerator::do_start);
  register_command("stop", &TriggerDecisionGenerator::do_stop);
  register_command("pause", &TriggerDecisionGenerator::do_pause);
  register_command("resume", &TriggerDecisionGenerator::do_resume);
}

void
TriggerDecisionGenerator::init(const nlohmann::json& iniobj)
{
}

void
TriggerDecisionGenerator::do_start(const nlohmann::json& /*startobj*/)
{
}

void
TriggerDecisionGenerator::do_stop(const nlohmann::json& /*stopobj*/)
{
}


} // namespace trigemu
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::trigemu::TriggerDecisionGenerator)
