// This is the application info schema used by the data link handler module.
// It describes the information object structure passed by the application 
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.trigemu.triggerdecisionemulatorinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("class_name", self.cl, "triggerdecisionemulatorinfo", doc="Info class name"),
       s.field("triggers", self.uint8, 0, doc="Integral trigger counter"), 
       s.field("new_triggers", self.uint8, 0, doc="Incremental trigger counter"), 
       s.field("inhibited", self.uint8, 0, doc="Number of triggers skipped"),
       s.field("new_inhibited", self.uint8, 0, doc="Incremental skipped counter"),
   ], doc="Trigger information information")
};

moo.oschema.sort_select(info) 
