// TOP: top
// TECH: nangate45
// TARGETS: deep_descendant_pin_collides_with_ancestor_port
// CLUE: dbReadVerilog.cc staToDb guards with
// `full_name == module->getHierarchicalName()` "to stop deep-descendant pins
// from falsely binding to same-named module ports", because findModBTerm
// matches by name only. Every level below reuses the ancestor's port names.
module top (clk, d, q);
   input clk, d;
   output q;
   mid u_mid (.clk(clk), .d(d), .q(q));
endmodule

module mid (clk, d, q);
   input clk, d;
   output q;
   leaf u_leaf (.clk(clk), .d(d), .q(q));
endmodule

module leaf (clk, d, q);
   input clk, d;
   output q;
   DFF_X1 r (.D(d), .CK(clk), .Q(q));
endmodule
