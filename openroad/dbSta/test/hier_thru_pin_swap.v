module sub (input clk, input a, input b, output q);
  wire n;
  XNOR2_X1 g (.A(a), .B(b), .ZN(n));
  DFF_X1 r2 (.D(n), .CK(clk), .Q(q));
endmodule

module top (input clk, input in1, input in2, output out, output out2);
  wire n_raw, n_mid, n_thru, n_other, n_buf;
  DFF_X1 r1 (.D(in1), .CK(clk), .Q(n_raw));
  BUF_X1 d1 (.A(n_raw), .Z(n_mid));
  BUF_X1 d2 (.A(n_mid), .Z(n_thru));
  DFF_X1 r3 (.D(in2), .CK(clk), .Q(n_other));
  BUF_X1 b1 (.A(n_thru), .Z(n_buf));
  DFF_X1 r4 (.D(n_buf), .CK(clk), .Q(out2));
  sub u (.clk(clk), .a(n_other), .b(n_thru), .q(out));
endmodule
