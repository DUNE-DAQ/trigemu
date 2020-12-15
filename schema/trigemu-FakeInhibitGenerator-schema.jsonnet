local moo = import "moo.jsonnet";
local ns = "dunedaq.trigemu.fakeinhibitgenerator";
local s = moo.oschema.schema(ns);

local types = {
  intervalms: s.number("interval_ms", dtype="i4"),
  
  start: s.record("start_params", [
    s.field("inhibit_interval_ms", self.intervalms, 5000,
      doc="Interval between XON/XOFF messages in ms"),
  ], doc="FakeInhibitGenerator start parameters"),
  
};

moo.oschema.sort_select(types, ns)
