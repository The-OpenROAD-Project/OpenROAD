// retime_basic with its register chain inside a submodule.
module chain (a, clk, y);
  input a;
  input clk;
  output y;
  wire q1, n1, n2, n3, n4;

  DFF_X1 r1 (.D(a), .CK(clk), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(y));
endmodule

module top (a, clk, y);
  input a;
  input clk;
  output y;
  wire a_buf, q;

  BUF_X1 b0 (.A(a), .Z(a_buf));
  chain u_chain (.a(a_buf), .clk(clk), .y(q));
  BUF_X1 b1 (.A(q), .Z(y));
endmodule
