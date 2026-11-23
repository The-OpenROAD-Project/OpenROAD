module top (
    input  clk,
    input  in,
    output out
);
  wire clk1;
  wire clk2;
  wire d1;
  wire q1;
  wire n1;
  wire n2;

  CLKBUF_X1 clk_buf1 (.A(clk), .Z(clk1));
  CLKBUF_X1 clk_buf2 (.A(clk1), .Z(clk2));

  BUF_X1 in_buf (.A(in), .Z(d1));
  DFF_X1 r1 (.D(d1), .CK(clk2), .Q(q1));
  BUF_X1 buf1 (.A(q1), .Z(n1));
  INV_X1 inv1 (.A(n1), .ZN(n2));
  DFF_X1 r2 (.D(n2), .CK(clk2), .Q(out));
endmodule
