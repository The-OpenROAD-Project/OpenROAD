module top(
  input clk, input clk2,
  input enable, input enable2, input enable3, input enable4,
  input d,
  output q, output q2,
  output gclk2, output gclk3
);
  wire gated_clk, gated_clk2, gated_clk3, gated_clk4;

  AND2_X1 clock_gate (.A1(clk), .A2(enable), .ZN(gated_clk));
  AND2_X1 clock_gate2 (.A1(gated_clk), .A2(enable2), .ZN(gated_clk2));
  BUF_X1 gated_clk2_buf (.A(gated_clk2), .Z(gclk2));
  DFF_X1 flop (.CK(gated_clk), .D(d), .Q(q), .QN());

  AND2_X1 clock_gate3 (.A1(clk2), .A2(enable3), .ZN(gated_clk3));
  BUF_X1 gated_clk3_buf (.A(gated_clk3), .Z(gclk3));
  AND2_X1 clock_gate4 (.A1(gated_clk3), .A2(enable4), .ZN(gated_clk4));
  DFF_X1 flop2 (.CK(gated_clk4), .D(d), .Q(q2), .QN());
endmodule