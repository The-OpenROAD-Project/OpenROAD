// TOP: top
// TECH: nangate45
// TARGETS: cellname_shadow, instance, depth_1
// CLUE: Instance name identical to its own cell type: INV_X1 INV_X1 (...).
module top (a, y);
  input a;
  output y;
  wire n;
  INV_X1 INV_X1 (.A(a), .ZN(n));
  INV_X1 u2 (.A(n), .ZN(y));
endmodule
