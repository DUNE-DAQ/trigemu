/**
 * @file Issues.hpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGEMU_INCLUDE_TRIGEMU_ISSUES_HPP_
#define TRIGEMU_INCLUDE_TRIGEMU_ISSUES_HPP_

#include "ers/Issue.hpp"

namespace dunedaq {
ERS_DECLARE_ISSUE(trigemu, InvalidTimeSync, "An invalid TimeSync message was received", ERS_EMPTY)

ERS_DECLARE_ISSUE(trigemu, InvalidConfiguration, "An invalid configuration object was received", ERS_EMPTY)

ERS_DECLARE_ISSUE(trigemu,
                  InvalidTriggerInterval,
                  "An invalid trigger interval of " << interval << " was requested",
                  ((uint64_t)interval)) // NOLINT(build/unsigned)
} // namespace dunedaq

#endif // TRIGEMU_INCLUDE_TRIGEMU_ISSUES_HPP_