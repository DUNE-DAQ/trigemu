local moo = import "moo.jsonnet";
local ns = "dunedaq.trigemu.triggerdecisionemulator";
local s = moo.oschema.schema(ns);

local types = {
  linkid: s.number("link_id", dtype="i4"),
  linkvec : s.sequence("link_vec", self.linkid),
  link_count: s.number("link_count", dtype="i4"),
  ticks: s.number("ticks", dtype="i8"),
  freq: s.number("frequency", dtype="u8"),
  
  conf : s.record("ConfParams", [
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
    s.field("trigger_window_offset", self.ticks, 1600,
      doc="Offset of trigger window start time in ticks before trigger timestamp"),
    s.field("trigger_interval_ticks", self.ticks, 64000000,
      doc="Interval between triggers in 16 ns time ticks (default 1.024 s) "),
    s.field("trigger_offset", self.ticks, 0,
      doc="Trigger timestamp modulo trigger_interval_ticks (default zero)"),
    s.field("trigger_delay_ticks", self.ticks, 5000000,
      doc="Number of ticks after timestamp occurs at which to emit trigger decision"),
    s.field("clock_frequency_hz", self.ticks, 50000000,
      doc="Assumed clock frequency in Hz (for current-timestamp estimation)"),
  ], doc="TriggerDecisionEmulator configuration parameters"),

  resume: s.record("ResumeParams", [
    s.field("trigger_interval_ticks", self.ticks, 64000000,
      doc="Interval between triggers in 16 ns time ticks (default 1.024 s)"),
  ], doc="TriggerDecisionEmulator resume parameters"),
  
};

moo.oschema.sort_select(types, ns)
