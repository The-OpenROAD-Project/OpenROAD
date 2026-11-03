module enable_reg (clk, en, d, q);
  input clk;
  input en;
  input [1:0] d;
  output [1:0] q;

  wire n0;
  wire n1;

  MUX2_X1 mux0 (.A(q[0]), .B(d[0]), .S(en), .Z(n0));
  MUX2_X1 mux1 (.A(q[1]), .B(d[1]), .S(en), .Z(n1));
  DFF_X1 r0 (.D(n0), .CK(clk), .Q(q[0]));
  DFF_X1 r1 (.D(n1), .CK(clk), .Q(q[1]));
endmodule
