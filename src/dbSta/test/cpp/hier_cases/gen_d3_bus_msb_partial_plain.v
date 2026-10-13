// TOP: top
// TECH: nangate45
// TARGETS: depth_3, bus_msb, partial, naming_plain
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 3 nesting,
// bus_msb ports wired partial, plain identifiers.

module top (ti, to);
 input [3:0] ti;
 output [3:0] to;

 gen_d3_bus_msb_partial_plain_w2 u_top (.wi2(ti), .wo2(to));
endmodule

module gen_d3_bus_msb_partial_plain_w2 (wi2, wo2);
 input [3:0] wi2;
 output [3:0] wo2;

 gen_d3_bus_msb_partial_plain_w1 u_child (.wi1(wi2), .wo1(wo2));
endmodule

module gen_d3_bus_msb_partial_plain_w1 (wi1, wo1);
 input [3:0] wi1;
 output [3:0] wo1;

 gen_d3_bus_msb_partial_plain_w0 u_child (.wi0(wi1), .wo0(wo1));
endmodule

module gen_d3_bus_msb_partial_plain_w0 (wi0, wo0);
 input [3:0] wi0;
 output [3:0] wo0;

 gen_d3_bus_msb_partial_plain_leaf u_child (.li(wi0), .lo(wo0));
endmodule

module gen_d3_bus_msb_partial_plain_leaf (li, lo);
 input [3:0] li;
 output [3:0] lo;

 INV_X1 g0 (.A(li[3]), .ZN(lo[3]));
 assign lo[2] = li[2];
 INV_X1 g2 (.A(li[1]), .ZN(lo[1]));
 assign lo[0] = li[0];
endmodule
