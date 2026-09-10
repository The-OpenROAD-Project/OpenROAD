// TOP: top
// TECH: nangate45
// TARGETS: tie0, depth_1
// CLUE: 1'b0 tie-off on a cell input pin at top level; writer must re-emit a
// constant driver (literal or LOGIC0 cell) for the tied pin.
module top (a, y);
  input a;
  output y;
  AND2_X1 u1 (.A1(a), .A2(1'b0), .ZN(y));
endmodule
