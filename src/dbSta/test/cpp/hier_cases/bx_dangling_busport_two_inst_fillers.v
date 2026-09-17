// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, two_instances, nc_filler_uniqueness
// CLUE: TWO instances each leave a 2-bit bus port unconnected. If the hier
// writer reuses _NC1/_NC2 for both, the two instances' inputs alias each other.
module sub (input a, input [1:0] db, output y);
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module top (input x1, input x2, output y1, output y2);
  sub u0 (.a(x1), .y(y1));
  sub u1 (.a(x2), .y(y2));
endmodule
