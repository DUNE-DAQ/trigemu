#include "ers/Issue.hpp"

namespace dunedaq {
ERS_DECLARE_ISSUE(trigemu, InvalidTimeSync, "An invalid TimeSync message was received", ERS_EMPTY)

ERS_DECLARE_ISSUE(trigemu, InvalidConfiguration, "An invalid configuration object was received", ERS_EMPTY)

ERS_DECLARE_ISSUE(trigemu, InvalidTriggerInterval, "An invalid trigger interval of " << interval << " was requested", ((uint64_t)interval))
} // dunedaq
