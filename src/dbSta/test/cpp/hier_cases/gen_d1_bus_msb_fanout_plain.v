// TOP: top
// TECH: nangate45
// TARGETS: depth_1, bus_msb, fanout, naming_plain
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// bus_msb ports wired fanout, plain identifiers.

module top (ti, to);
 input [3:0] ti;
 output [3:0] to;

 gen_d1_bus_msb_fanout_plain_w0 u_top (.wi0(ti), .wo0(to));
endmodule

module gen_d1_bus_msb_fanout_plain_w0 (wi0, wo0);
 input [3:0] wi0;
 output [3:0] wo0;

 gen_d1_bus_msb_fanout_plain_leaf u_child (.li(wi0), .lo(wo0));
endmodule

module gen_d1_bus_msb_fanout_plain_leaf (li, lo);
 input [3:0] li;
 output [3:0] lo;

 BUF_X1 g0 (.A(li[3]), .Z(lo[3]));
 BUF_X1 g1 (.A(li[3]), .Z(lo[2]));
 BUF_X1 g2 (.A(li[3]), .Z(lo[1]));
 BUF_X1 g3 (.A(li[3]), .Z(lo[0]));
endmodule
