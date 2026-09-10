// TOP: top
// TECH: nangate45
// TARGETS: cellname_shadow, instance, depth_1
// CLUE: Instance named INV_X1 of type BUF_X1 and instance named BUF_X1 of type INV_X1, next to normally-named cells.
module top (a, y);
  input a;
  output y;
  wire n1, n2, n3;
  BUF_X1 INV_X1 (.A(a), .Z(n1));
  INV_X1 BUF_X1 (.A(n1), .ZN(n2));
  INV_X1 u_real (.A(n2), .ZN(n3));
  BUF_X1 u_real2 (.A(n3), .Z(y));
endmodule
