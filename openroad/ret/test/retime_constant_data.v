// rt holds a constant: a tie cell feeds it.  rt and r1 feed the full adder
// g1 in front of a failing cone of inverters.
module top (a, rst_n, clk, y);
  input a;
  input rst_n;
  input clk;
  output y;
  wire c0, qt, d1, q1, n1, n2, n3, n4, n5, n6, n7, d3, q3, x1;

  LOGIC0_X1 t0 (.Z(c0));
  DFF_X1 rt (.D(c0), .CK(clk), .Q(qt));
  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  FA_X1 g1 (.A(qt), .B(q1), .CI(q1), .S(n1), .CO(x1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  INV_X1 g5 (.A(n4), .ZN(n5));
  INV_X1 g6 (.A(n5), .ZN(n6));
  INV_X1 g7 (.A(n6), .ZN(n7));
  AND2_X1 g8 (.A1(n7), .A2(rst_n), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(clk), .Q(q3));
  BUF_X1 g9 (.A(q3), .Z(y));
endmodule
