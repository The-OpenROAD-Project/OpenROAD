// TOP: top
// TECH: nangate45
// TARGETS: tie1, depth_1
// CLUE: single pin tied to literal 1'b1 at top level; writer may re-express
// as LOGIC1 cell, assign, or dedicated tie net.
module top (a, y);
  input a;
  output y;
  NAND2_X1 u1 (.A1(a), .A2(1'b1), .ZN(y));
endmodule
