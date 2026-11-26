// As retime_basic, with a second driver (g9) on the net into r1.
module top (a, b, f, clk, y);
  input a;
  input b;
  input f;
  input clk;
  output y;
  wire bus, q1, n1, n2, n3, n4, q2;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(bus));
  INV_X1 g9 (.A(f), .ZN(bus));
  DFF_X1 r1 (.D(bus), .CK(clk), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y));
endmodule
