// TOP: top
// TECH: nangate45
// TARGETS: depth_1, bus_split, partial, naming_plain
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// bus_split ports wired partial, plain identifiers.

module top (ti, to);
 input [3:0] ti;
 output [3:0] to;

 gen_d1_bus_split_partial_plain_w0 u_top (.wi0(ti), .wo0(to));
endmodule

module gen_d1_bus_split_partial_plain_w0 (wi0, wo0);
 input [3:0] wi0;
 output [3:0] wo0;

 gen_d1_bus_split_partial_plain_leaf u_child (.li(wi0), .lo(wo0));
endmodule

module gen_d1_bus_split_partial_plain_leaf (li, lo);
 input [3:0] li;
 output [3:0] lo;

 INV_X1 g0 (.A(li[3]), .ZN(lo[3]));
 assign lo[2] = li[2];
 INV_X1 g2 (.A(li[1]), .ZN(lo[1]));
 assign lo[0] = li[0];
endmodule
