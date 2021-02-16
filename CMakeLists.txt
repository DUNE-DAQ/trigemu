cmake_minimum_required(VERSION 3.12)
project(trigemu VERSION 1.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(logging REQUIRED)
find_package(dfmessages REQUIRED)

set(TRIGEMU_DEPENDENCIES appfwk::appfwk logging::logging dfmessages::dfmessages)

daq_add_plugin(TriggerDecisionEmulator duneDAQModule LINK_LIBRARIES ${TRIGEMU_DEPENDENCIES} SCHEMA)

daq_add_plugin(FakeTimeSyncSource duneDAQModule LINK_LIBRARIES ${TRIGEMU_DEPENDENCIES} SCHEMA)
daq_add_plugin(FakeInhibitGenerator duneDAQModule LINK_LIBRARIES ${TRIGEMU_DEPENDENCIES} TEST SCHEMA)
daq_add_plugin(FakeRequestReceiver duneDAQModule LINK_LIBRARIES ${TRIGEMU_DEPENDENCIES} TEST)

daq_install()
