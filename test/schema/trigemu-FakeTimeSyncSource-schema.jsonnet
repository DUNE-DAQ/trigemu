local moo = import "moo.jsonnet";
local ns = "dunedaq.trigemu.faketimesyncsource";
local s = moo.oschema.schema(ns);

local types = {
  ticks: s.number("ticks", dtype="i8"),
  
  start: s.record("ConfParams", [
    s.field("sync_interval_ticks", self.ticks, 50000000,
      doc="Interval between timesyncs in clock ticks (default 1.0 s) "),
    s.field("clock_frequency_hz", self.ticks, 50000000,
      doc="Clock frequency in Hz"),
  ], doc="FakeTimeSyncSource start parameters"),
  
};

moo.oschema.sort_select(types, ns)
