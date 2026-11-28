// retime_basic with the first inverter named h1, so that it comes after g2
// in the block: the gate a fence pins is then visited after the gate that
// pins it.
module top (a, b, clk, y);
  input a;
  input b;
  input clk;
  output y;
  wire n0, q1, n1, n2, n3, n4, q2;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));
  DFF_X1 r1 (.D(n0), .CK(clk), .Q(q1));
  INV_X1 h1 (.A(q1), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y));
endmodule
