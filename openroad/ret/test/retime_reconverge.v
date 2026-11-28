// k is a scan flip-flop the library does not mark as one.  g0 reaches it
// twice: by SI directly and by D one register later, through r1.  The path
// from r1 fails and would slide r1 across the inverters, which would take
// r1 from in front of k/D as well.
module top (a, b, se, clk, y, z);
  input a;
  input b;
  input se;
  input clk;
  output y;
  output z;
  wire n0, q1, qk, n1, n2, n3, n4, q2;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));
  DFF_X1 r1 (.D(n0), .CK(clk), .Q(q1));
  SDFF_X1 k (.D(q1), .SI(n0), .SE(se), .CK(clk), .Q(qk));
  BUF_X1 g6 (.A(qk), .Z(z));
  INV_X1 g1 (.A(q1), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y));
endmodule
