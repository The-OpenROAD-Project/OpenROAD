// Two registers on two clock ports: retime refuses multi-clock designs.
module top (a, clk1, clk2, y);
  input a;
  input clk1;
  input clk2;
  output y;
  wire n0, q1, n1, q2;

  INV_X1 g0 (.A(a), .ZN(n0));
  DFF_X1 r1 (.D(n0), .CK(clk1), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(n1));
  DFF_X1 r2 (.D(n1), .CK(clk2), .Q(q2));
  BUF_X1 g2 (.A(q2), .Z(y));
endmodule
