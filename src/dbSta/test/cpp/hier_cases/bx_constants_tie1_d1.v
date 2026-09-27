// TOP: top
// TECH: nangate45
// TARGETS: tie1, depth_1
// CLUE: 1'b1 tie-off on a cell input pin at top level.
module top (a, y);
  input a;
  output y;
  OR2_X1 u1 (.A1(a), .A2(1'b1), .ZN(y));
endmodule
