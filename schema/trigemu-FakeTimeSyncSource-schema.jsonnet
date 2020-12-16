local moo = import "moo.jsonnet";
local ns = "dunedaq.trigemu.faketimesyncsource";
local s = moo.oschema.schema(ns);

local types = {
  ticks: s.number("ticks", dtype="i8"),
  
  start: s.record("ConfParams", [
    s.field("sync_interval_ticks", self.ticks, 64000000,
      doc="Interval between timesyncs in 16 ns time ticks (default 1.024 s) "),
  ], doc="FakeTimeSyncSource start parameters"),
  
};

moo.oschema.sort_select(types, ns)
