// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, nc_filler_vs_live_scalar
// CLUE: the single user net named _NC1 is on the live path (driven by sub's y,
// consumed by an INV that drives the top output), and the same instance
// leaves its 2-bit db open. The filler must not merge with it.
module sub (input a, input [1:0] db, output y);
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module top (input x, output y);
  wire _NC1;
  sub u0 (.a(x), .y(_NC1));
  INV_X1 g1 (.A(_NC1), .ZN(y));
endmodule
