// TOP: top
// TECH: nangate45
// TARGETS: tie0, depth_1
// CLUE: single pin tied to literal 1'b0 at top level; writer may re-express
// as LOGIC0 cell, assign, or dedicated tie net.
module top (a, y);
  input a;
  output y;
  NOR2_X1 u1 (.A1(a), .A2(1'b0), .ZN(y));
endmodule
