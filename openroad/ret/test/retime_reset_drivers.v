// The reset of r1 is on a net that a tie cell and an inverter both drive.
// The tie cell alone would hold the reset inactive; the inverter can assert
// it, so the reset is live and r1 stays, with the failing path behind it.
module top (a, rst_n, clk, y);
  input a;
  input rst_n;
  input clk;
  output y;
  wire rn, d1, q1, n1, n2, n3, n4, q2;

  LOGIC1_X1 t1 (.Z(rn));
  INV_X1 gx (.A(rst_n), .ZN(rn));
  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  DFFR_X1 r1 (.D(d1), .RN(rn), .CK(clk), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y));
endmodule
