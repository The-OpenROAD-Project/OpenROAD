// TOP: top$1
// TECH: nangate45
// TARGETS: dollar_name, top_name
// CLUE: Top module itself named with a $ (top$1); link_design and writer must handle a $ in the design name.
module top$1 (a, y);
  input a;
  output y;
  wire n;
  INV_X1 u1 (.A(a), .ZN(n));
  INV_X1 u2 (.A(n), .ZN(y));
endmodule
