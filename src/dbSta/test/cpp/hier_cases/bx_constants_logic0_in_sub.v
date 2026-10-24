// TOP: top
// TECH: nangate45
// TARGETS: logic_cells, submodule, boundary_crossing
// CLUE: LOGIC0_X1 lives inside a submodule and its constant crosses the module
// boundary upward through an output port to be consumed at top.
module sub (output c);
  LOGIC0_X1 l0 (.Z(c));
endmodule

module top (input a, output y);
  wire t;
  sub s1 (.c(t));
  OR2_X1 g (.A1(a), .A2(t), .ZN(y));
endmodule
