// TOP: top
// TECH: nangate45
// TARGETS: dollar_name, instance, depth_1
// CLUE: $ in plain cell instance names (u$1, i$$, g$x$) at top level.
module top (a, b, y);
  input a, b;
  output y;
  wire n1, n2;
  NAND2_X1 u$1 (.A1(a), .A2(b), .ZN(n1));
  INV_X1 i$$ (.A(n1), .ZN(n2));
  BUF_X1 g$x$ (.A(n2), .Z(y));
endmodule
