cmake_minimum_required(VERSION 3.12)
project(trigemu VERSION 1.0.0)

find_package(daq-cmake REQUIRED )

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(dfmessages REQUIRED)

daq_add_plugin(TriggerDecisionEmulator duneDAQModule LINK_LIBRARIES appfwk::appfwk dfmessages::dfmessages SCHEMA)

daq_add_plugin(FakeTimeSyncSource duneDAQModule LINK_LIBRARIES appfwk::appfwk dfmessages::dfmessages TEST SCHEMA)

daq_install()