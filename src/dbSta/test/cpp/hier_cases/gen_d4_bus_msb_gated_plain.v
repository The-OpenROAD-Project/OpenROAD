// TOP: top
// TECH: nangate45
// TARGETS: depth_4, bus_msb, gated, naming_plain
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 4 nesting,
// bus_msb ports wired gated, plain identifiers.

module top (ti, to);
 input [3:0] ti;
 output [3:0] to;

 gen_d4_bus_msb_gated_plain_w3 u_top (.wi3(ti), .wo3(to));
endmodule

module gen_d4_bus_msb_gated_plain_w3 (wi3, wo3);
 input [3:0] wi3;
 output [3:0] wo3;

 gen_d4_bus_msb_gated_plain_w2 u_child (.wi2(wi3), .wo2(wo3));
endmodule

module gen_d4_bus_msb_gated_plain_w2 (wi2, wo2);
 input [3:0] wi2;
 output [3:0] wo2;

 gen_d4_bus_msb_gated_plain_w1 u_child (.wi1(wi2), .wo1(wo2));
endmodule

module gen_d4_bus_msb_gated_plain_w1 (wi1, wo1);
 input [3:0] wi1;
 output [3:0] wo1;

 gen_d4_bus_msb_gated_plain_w0 u_child (.wi0(wi1), .wo0(wo1));
endmodule

module gen_d4_bus_msb_gated_plain_w0 (wi0, wo0);
 input [3:0] wi0;
 output [3:0] wo0;

 gen_d4_bus_msb_gated_plain_leaf u_child (.li(wi0), .lo(wo0));
endmodule

module gen_d4_bus_msb_gated_plain_leaf (li, lo);
 input [3:0] li;
 output [3:0] lo;

 BUF_X1 g0 (.A(li[3]), .Z(lo[3]));
 BUF_X1 g1 (.A(li[2]), .Z(lo[2]));
 BUF_X1 g2 (.A(li[1]), .Z(lo[1]));
 BUF_X1 g3 (.A(li[0]), .Z(lo[0]));
endmodule
