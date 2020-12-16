# Compile this file to json with moo compile, eg:
# moo -M $DBT_AREA_ROOT/sourcecode/appfwk/schema compile $DBT_AREA_ROOT/sourcecode/trigemu/schema/fake-job.jsonnet > $DBT_AREA_ROOT/sourcecode/trigemu/schema/fake-job.json

local moo = import "moo.jsonnet";

local cmd = import "appfwk-cmd-make.jsonnet";
#local ftss = import "trigemu-FakeTimeSyncsource-make.jsonnet";

local time_sync_q = "time_sync_q";
local trigger_inhibit_q = "trigger_inhibit_q";
local trigger_decision_q = "trigger_decision_q";

[
  cmd.init(
    [
      cmd.qspec(time_sync_q, "FollyMPMCQueue", 10000),
      cmd.qspec(trigger_inhibit_q, "FollySPSCQueue", 10000),
      cmd.qspec(trigger_decision_q, "FollySPSCQueue", 10000),
    ],
    [
      cmd.mspec("ftss", "FakeTimeSyncSource",
        cmd.qinfo("time_sync_sink", time_sync_q, cmd.qdir.output)),

      cmd.mspec("fig", "FakeInhibitGenerator",
        cmd.qinfo("trigger_inhibit_sink", trigger_inhibit_q, cmd.qdir.output)),

      cmd.mspec("frr", "FakeRequestReceiver",
        cmd.qinfo("trigger_decision_source", trigger_decision_q, cmd.qdir.input)),

      cmd.mspec("tde", "TriggerDecisionEmulator",
        [
          cmd.qinfo("time_sync_source", time_sync_q, cmd.qdir.input),
          cmd.qinfo("trigger_inhibit_source", trigger_inhibit_q, cmd.qdir.input),
          cmd.qinfo("trigger_decision_sink", trigger_decision_q, cmd.qdir.output),
        ]
      ),
    ]
  ),

  cmd.conf(
    [
      # PAR 2020-12-16 FakeRequestReceiver doesn't have a config function
      # cmd.mcmd("frr", {}),
      cmd.mcmd("ftss", {}),
      cmd.mcmd("fig", {}),
      cmd.mcmd("tde",
        {
          "links" : [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
          "min_links_in_request" : 2,
          "max_links_in_request" : 10,
          "min_readout_window_ticks" : 320000,
          "max_readout_window_ticks" : 320000,
        }
      ),
    ]
  ),

  # PAR 2020-12-15: cmd.start (from appfwk-cmd-make.jsonnet) assumes
  # that it's only being passed a run number, which goes to all
  # modules, but Giovanna's design assumes we pass an arbitrary object
  # for run. That's above my pay grade, so create the command
  # "manually"
  
  cmd.cmd(
    "start",
    [
      cmd.mcmd("frr", {}),
      cmd.mcmd("ftss",
        {
          "sync_interval_ticks": 64000000
        }
      ),
      cmd.mcmd("fig",
        {
          "inhibit_interval_ms": 5000
        }
      ),
      cmd.mcmd("tde",
        {
          "trigger_interval_ticks" : 64000000
        }
      )
    ]
  ),

  cmd.stop(),

]
