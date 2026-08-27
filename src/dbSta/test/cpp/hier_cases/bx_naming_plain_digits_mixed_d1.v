// TOP: top
// TECH: nangate45
// TARGETS: digits_in_name, net, instance, port, depth_1
// CLUE: Digit-heavy legal names (a1b2, n0q9, x0123456789, g4x5) on nets, instances and ports.
module top (a1, b2, y9);
  input a1, b2;
  output y9;
  wire a1b2, n0q9, x0123456789;
  AND2_X1 g4x5 (.A1(a1), .A2(b2), .ZN(a1b2));
  INV_X1 u2u2 (.A(a1b2), .ZN(n0q9));
  BUF_X1 u3 (.A(n0q9), .Z(x0123456789));
  INV_X1 u4 (.A(x0123456789), .ZN(y9));
endmodule
