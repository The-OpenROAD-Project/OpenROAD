// g1 and g3 drive the net bus together.  r1 in front of g1 fails; sliding it
// across g1 would put the new register behind bus, and what g3 drives would
// reach i1 one cycle late.
module top (a, b, f, clk, y);
  input a;
  input b;
  input f;
  input clk;
  output y;
  wire d1, q1, bus, n1, n2, n3, n4, q2;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(d1));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(bus));
  INV_X1 g3 (.A(f), .ZN(bus));
  INV_X1 i1 (.A(bus), .ZN(n1));
  INV_X1 i2 (.A(n1), .ZN(n2));
  INV_X1 i3 (.A(n2), .ZN(n3));
  INV_X1 i4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y));
endmodule
