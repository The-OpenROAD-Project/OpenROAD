// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, feedthrough_plus_internal_load
// CLUE: the input slice that feeds the feedthrough assign is ALSO consumed by an internal gate of the submodule.

module sub (input [3:0] a, output [1:0] y, output g);
  assign y = a[3:2];
  NAND2_X1 g0 (.A1(a[3]), .A2(a[2]), .ZN(g));
endmodule

module top (input [3:0] i, output [1:0] o, output og);
  sub u0 (.a(i), .y(o), .g(og));
endmodule
