local moo = import "moo.jsonnet";
local ns = "dunedaq.trigemu.faketokengenerator";
local s = moo.oschema.schema(ns);

local types = {
  intervalms: s.number("interval_ms", dtype="i4"),
  sigmams: s.number("sigma_ms", dtype="i4"),
  
  conf: s.record("ConfParams", [
    s.field("token_interval_ms", self.intervalms, 1000, doc="Interval between token messages in ms"),
      s.field("token_sigma_ms", self.sigmams, 0, doc="Variance of interval between token messages"),
  ], doc="FakeTokenGenerator conf parameters"),
  
};

moo.oschema.sort_select(types, ns)
