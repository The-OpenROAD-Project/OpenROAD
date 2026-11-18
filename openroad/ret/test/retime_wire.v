// r0 and g0 sit far from r1 and the inverter chain behind it, so the net from
// g0 to r1 is a long wire.  Moving r1 across h1 would put that wire on the
// path from g0 to h1.
module top (a, clk, y);
  input a;
  input clk;
  output y;
  wire d0, q0, n0, q1, m1, m2, m3, m4, q2;

  BUF_X1 gi (.A(a), .Z(d0));
  DFF_X1 r0 (.D(d0), .CK(clk), .Q(q0));
  INV_X1 g0 (.A(q0), .ZN(n0));
  DFF_X1 r1 (.D(n0), .CK(clk), .Q(q1));
  INV_X1 h1 (.A(q1), .ZN(m1));
  INV_X1 h2 (.A(m1), .ZN(m2));
  INV_X1 h3 (.A(m2), .ZN(m3));
  INV_X1 h4 (.A(m3), .ZN(m4));
  DFF_X1 r2 (.D(m4), .CK(clk), .Q(q2));
  BUF_X1 go (.A(q2), .Z(y));
endmodule
