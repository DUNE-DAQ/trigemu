local moo = import "moo.jsonnet";
local ns = "dunedaq.trigemu.triggerdecisionemulator";
local s = moo.oschema.schema(ns);

local types = {
  linkid: s.number("link_id", dtype="i4"),
  linkvec : s.sequence("link_vec", self.linkid),
  link_count: s.number("link_count", dtype="i4"),
  ticks: s.number("ticks", dtype="i8"),
  
  conf : s.record("conf_params", [
    s.field("links", self.linkvec,
      doc="List of link identifiers that may be included into trigger decision"),
    s.field("min_links_in_request", self.link_count, 10,
      doc="Minimum number of links to include in the trigger decision"),
    s.field("max_links_in_request", self.link_count, 10,
      doc="Maximum number of links to include in the trigger decision"),
    s.field("min_readout_window_ticks", self.ticks, 3200,
      doc="Minimum readout window to ask data for in 16 ns time ticks (default 51.2 us)"),
    s.field("max_readout_window_ticks", self.ticks, 320000,
      doc="Maximum readout window to ask data for in 16 ns time ticks (default 5.12 ms)"),
  ], doc="TriggerDecisionEmulator configuration parameters"),

  start: s.record("start_params", [
    s.field("trigger_interval_ticks", self.ticks, 64000000,
      doc="Interval between triggers in 16 ns time ticks (default 1.024 s) "),
  ], doc="TriggerDecisionEmulator start parameters"),
  
  resume: s.record("resume_params", [
    s.field("trigger_interval_ticks", self.ticks, 64000000,
      doc="Interval between triggers in 16 ns time ticks (default 1.024 s)"),
  ], doc="TriggerDecisionEmulator resume parameters"),
  
};

moo.oschema.sort_select(types, ns)
