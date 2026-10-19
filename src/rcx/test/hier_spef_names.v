module sub (clk_in, d, q);
  input clk_in;
  input d;
  output q;
  wire n1;

  BUF_X1 u_root (.A(clk_in), .Z(n1));
  DFF_X1 ff (.CK(n1), .D(d), .Q(q));
endmodule

module top (clk, d, q);
  input clk;
  input d;
  output q;

  sub u_sub (.clk_in(clk), .d(d), .q(q));
endmodule
