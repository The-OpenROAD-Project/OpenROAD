// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, nc_filler_vs_top_output
// CLUE: the _NC filler name collides with a live top OUTPUT port named _NC1: the
// emitted netlist ends up driving the submodule's dangling bus bit from
// that output net. LEC cannot see it because db is unused inside sub.
module sub (input a, input [1:0] db, output y);
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module top (input x, output _NC1, output y);
  INV_X1 g0 (.A(x), .ZN(_NC1));
  sub u0 (.a(x), .y(y));
endmodule
