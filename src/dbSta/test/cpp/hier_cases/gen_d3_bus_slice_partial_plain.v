// TOP: top
// TECH: nangate45
// TARGETS: depth_3, bus_slice, partial, naming_plain
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 3 nesting,
// bus_slice ports wired partial, plain identifiers.

module top (ti, to);
 input [3:0] ti;
 output [3:0] to;

 gen_d3_bus_slice_partial_plain_w2 u_top (.wi2(ti), .wo2(to));
endmodule

module gen_d3_bus_slice_partial_plain_w2 (wi2, wo2);
 input [3:0] wi2;
 output [3:0] wo2;

 gen_d3_bus_slice_partial_plain_w1 u_child (.wi1({wi2[3], wi2[2], wi2[1], wi2[0]}),
    .wo1({wo2[3], wo2[2], wo2[1], wo2[0]}));
endmodule

module gen_d3_bus_slice_partial_plain_w1 (wi1, wo1);
 input [3:0] wi1;
 output [3:0] wo1;

 gen_d3_bus_slice_partial_plain_w0 u_child (.wi0({wi1[3], wi1[2], wi1[1], wi1[0]}),
    .wo0({wo1[3], wo1[2], wo1[1], wo1[0]}));
endmodule

module gen_d3_bus_slice_partial_plain_w0 (wi0, wo0);
 input [3:0] wi0;
 output [3:0] wo0;

 gen_d3_bus_slice_partial_plain_leaf u_child (.li({wi0[3], wi0[2], wi0[1], wi0[0]}),
    .lo({wo0[3], wo0[2], wo0[1], wo0[0]}));
endmodule

module gen_d3_bus_slice_partial_plain_leaf (li, lo);
 input [3:0] li;
 output [3:0] lo;

 INV_X1 g0 (.A(li[3]), .ZN(lo[3]));
 assign lo[2] = li[2];
 INV_X1 g2 (.A(li[1]), .ZN(lo[1]));
 assign lo[0] = li[0];
endmodule
