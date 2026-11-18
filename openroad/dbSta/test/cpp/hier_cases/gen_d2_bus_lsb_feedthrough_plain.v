// TOP: top
// TECH: nangate45
// TARGETS: depth_2, bus_lsb, feedthrough, naming_plain
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 2 nesting,
// bus_lsb ports wired feedthrough, plain identifiers.

module top (ti, to);
 input [0:3] ti;
 output [0:3] to;

 gen_d2_bus_lsb_feedthrough_plain_w1 u_top (.wi1(ti), .wo1(to));
endmodule

module gen_d2_bus_lsb_feedthrough_plain_w1 (wi1, wo1);
 input [0:3] wi1;
 output [0:3] wo1;

 gen_d2_bus_lsb_feedthrough_plain_w0 u_child (.wi0(wi1), .wo0(wo1));
endmodule

module gen_d2_bus_lsb_feedthrough_plain_w0 (wi0, wo0);
 input [0:3] wi0;
 output [0:3] wo0;

 gen_d2_bus_lsb_feedthrough_plain_leaf u_child (.li(wi0), .lo(wo0));
endmodule

module gen_d2_bus_lsb_feedthrough_plain_leaf (li, lo);
 input [0:3] li;
 output [0:3] lo;

 assign lo[0] = li[0];
 assign lo[1] = li[1];
 assign lo[2] = li[2];
 assign lo[3] = li[3];
endmodule
