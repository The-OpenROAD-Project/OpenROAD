// A feedback loop with two registers: rL1 -> XOR -> ten inverters -> rL2 ->
// AND -> rL1.  The loop is unbalanced (eleven gates on one side, one on the
// other), so rL1 slides forward into the inverter chain together with rIn,
// the register on the XOR's other input; the number of registers on the loop
// stays two.  rL2 also drives the output y, so it stays where it is.  The
// reset reaches the loop through register rR, which keeps the loop's AND
// gate away from the primary inputs, and every register settles to a defined
// value for the sequential equivalence check.
module top (a, rst_n, clk, y);
  input a;
  input rst_n;
  input clk;
  output y;
  wire rb, q_rst, d_in, q_in, q_l2, d_l1, q_l1;
  wire x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10;

  BUF_X1 g_r (.A(rst_n), .Z(rb));
  DFF_X1 rR (.D(rb), .CK(clk), .Q(q_rst));
  AND2_X1 g_in (.A1(a), .A2(rst_n), .ZN(d_in));
  DFF_X1 rIn (.D(d_in), .CK(clk), .Q(q_in));

  XOR2_X1 g_x (.A(q_l1), .B(q_in), .Z(x0));
  INV_X1 i1 (.A(x0), .ZN(x1));
  INV_X1 i2 (.A(x1), .ZN(x2));
  INV_X1 i3 (.A(x2), .ZN(x3));
  INV_X1 i4 (.A(x3), .ZN(x4));
  INV_X1 i5 (.A(x4), .ZN(x5));
  INV_X1 i6 (.A(x5), .ZN(x6));
  INV_X1 i7 (.A(x6), .ZN(x7));
  INV_X1 i8 (.A(x7), .ZN(x8));
  INV_X1 i9 (.A(x8), .ZN(x9));
  INV_X1 i10 (.A(x9), .ZN(x10));
  DFF_X1 rL2 (.D(x10), .CK(clk), .Q(q_l2));
  AND2_X1 g_and (.A1(q_l2), .A2(q_rst), .ZN(d_l1));
  DFF_X1 rL1 (.D(d_l1), .CK(clk), .Q(q_l1));
  BUF_X1 g_o (.A(q_l2), .Z(y));
endmodule
