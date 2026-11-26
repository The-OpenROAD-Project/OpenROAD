// t1 is a tri-state driver with a register on each input.  Its output is not
// a function of its inputs alone, so no register crosses it.
module top (a, b, e, clk, y);
  input a;
  input b;
  input e;
  input clk;
  output y;
  wire d1, q1, de, qe, n0, n1, n2, n3, n4, q2;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(d1));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  BUF_X1 g1 (.A(e), .Z(de));
  DFF_X1 re (.D(de), .CK(clk), .Q(qe));
  TBUF_X1 t1 (.A(q1), .EN(qe), .Z(n0));
  INV_X1 i1 (.A(n0), .ZN(n1));
  INV_X1 i2 (.A(n1), .ZN(n2));
  INV_X1 i3 (.A(n2), .ZN(n3));
  INV_X1 i4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y));
endmodule
