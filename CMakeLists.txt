cmake_minimum_required(VERSION 3.12)
project(trigemu VERSION 2.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(logging REQUIRED)
find_package(dfmessages REQUIRED)

daq_codegen(*.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 )

daq_add_library(TimestampEstimator.cpp LINK_LIBRARIES appfwk::appfwk dfmessages::dfmessages)

daq_add_plugin(TriggerDecisionEmulator duneDAQModule LINK_LIBRARIES trigemu)

daq_add_plugin(FakeTimeSyncSource duneDAQModule LINK_LIBRARIES trigemu)
daq_add_plugin(FakeInhibitGenerator duneDAQModule LINK_LIBRARIES trigemu TEST)
daq_add_plugin(FakeTokenGenerator duneDAQModule LINK_LIBRARIES trigemu TEST)
daq_add_plugin(FakeRequestReceiver duneDAQModule LINK_LIBRARIES trigemu TEST)

daq_install()
