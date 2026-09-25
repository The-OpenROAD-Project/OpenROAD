// TOP: top
// TECH: nangate45
// TARGETS: depth_2, bus_msb, gated, naming_plain
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 2 nesting,
// bus_msb ports wired gated, plain identifiers.

module top (ti, to);
 input [3:0] ti;
 output [3:0] to;

 gen_d2_bus_msb_gated_plain_w1 u_top (.wi1(ti), .wo1(to));
endmodule

module gen_d2_bus_msb_gated_plain_w1 (wi1, wo1);
 input [3:0] wi1;
 output [3:0] wo1;

 gen_d2_bus_msb_gated_plain_w0 u_child (.wi0(wi1), .wo0(wo1));
endmodule

module gen_d2_bus_msb_gated_plain_w0 (wi0, wo0);
 input [3:0] wi0;
 output [3:0] wo0;

 gen_d2_bus_msb_gated_plain_leaf u_child (.li(wi0), .lo(wo0));
endmodule

module gen_d2_bus_msb_gated_plain_leaf (li, lo);
 input [3:0] li;
 output [3:0] lo;

 BUF_X1 g0 (.A(li[3]), .Z(lo[3]));
 BUF_X1 g1 (.A(li[2]), .Z(lo[2]));
 BUF_X1 g2 (.A(li[1]), .Z(lo[1]));
 BUF_X1 g3 (.A(li[0]), .Z(lo[0]));
endmodule
