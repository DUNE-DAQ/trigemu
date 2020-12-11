local moo = import "moo.jsonnet";
local ns = "dunedaq.trigemu.TriggerDecisionEmulator";
local s = moo.oschema.schema(ns);

local types = {
    links : s.sequence("linkId", s.number(dtype="i4")),

    conf : s.record("ConfParams", [
        s.field("links", links,
                doc="List of link identifiers that may be included into trigger decision."),
        s.field("min_links_in_request", s.number(dtype="i4"), 10,
		doc="Minimum number of links to include in the trigger decision".),
        s.field("max_links_in_request", s.number(dtype="i4"), 10,
                doc="Maximum number of links to include in the trigger decision".),
       s.field("min_readout_window_ticks", s.number(dtype="i4"), 3200,
                doc="Minimum readout window to ask data for in 16 ns time ticks (default 51.2 us)".),
       s.field("max_readout_window_ticks", s.number(dtype="i4"), 320000,
                doc="Maximum readout window to ask data for in 16 ns time ticks (default 5.12 ms)".),
    ], doc="TriggerDecisionEmulator configuration parameters"),

    start: s.record("StartParams"[
	s.field("trigger_interval_ticks", s.number(dtype="i4"), 64000000,
                doc="Interval between triggers in 16 ns time ticks (default 1.024 s) ".),
    ], doc="TriggerDecisionEmulator start parameters"),

    resume: s.record("ResumeParams"[
        s.field("trigger_interval_ticks", s.number(dtype="i4"), 64000000,
                doc="Interval between triggers in 16 ns time ticks (default 1.024 s)".),
    ], doc="TriggerDecisionEmulator resume parameters"),

};

moo.oschema.sort_select(types, ns)
