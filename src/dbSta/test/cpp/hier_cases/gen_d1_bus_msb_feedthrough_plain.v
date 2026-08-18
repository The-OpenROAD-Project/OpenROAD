// TOP: top
// TECH: nangate45
// TARGETS: depth_1, bus_msb, feedthrough, naming_plain
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// bus_msb ports wired feedthrough, plain identifiers.

module top (ti, to);
 input [3:0] ti;
 output [3:0] to;

 gen_d1_bus_msb_feedthrough_plain_w0 u_top (.wi0(ti), .wo0(to));
endmodule

module gen_d1_bus_msb_feedthrough_plain_w0 (wi0, wo0);
 input [3:0] wi0;
 output [3:0] wo0;

 gen_d1_bus_msb_feedthrough_plain_leaf u_child (.li(wi0), .lo(wo0));
endmodule

module gen_d1_bus_msb_feedthrough_plain_leaf (li, lo);
 input [3:0] li;
 output [3:0] lo;

 assign lo[3] = li[3];
 assign lo[2] = li[2];
 assign lo[1] = li[1];
 assign lo[0] = li[0];
endmodule
