module top(
  input clk, input clk2,
  input enable, input enable2, input enable3, input enable4,
  input d, input d2,
  output q, output q2, output q3, output q4,
  output gclk2, output gclk3
);
  wire gated_clk, gated_clk2, gated_clk3, gated_clk4;

  // clk gating endpoint on A2, cell in the fanin of endpoint cell
  AND2_X1 clock_gate (.A1(clk), .A2(enable), .ZN(gated_clk));
  // clk gating endpoint on A2, cell in the fanin of non-endpoint cell
  AND2_X1 clock_gate2 (.A1(gated_clk), .A2(enable2), .ZN(gated_clk2));
  BUF_X1 gated_clk2_buf (.A(gated_clk2), .Z(gclk2));
  DFF_X1 flop (.CK(gated_clk), .D(d), .Q(q), .QN());

  // clk gating endpoint on A2, cell in the fanins of endpoint and non-endpoint cells
  AND2_X1 clock_gate3 (.A1(clk2), .A2(enable3), .ZN(gated_clk3));
  BUF_X1 gated_clk3_buf (.A(gated_clk3), .Z(gclk3));
  // clk gating endpoint on A2, cell in the fanin of non-endpoint on DFF
  AND2_X1 clock_gate4 (.A1(gated_clk3), .A2(enable4), .ZN(gated_clk4));
  DFF_X1 flop2 (.CK(gated_clk4), .D(d), .Q(q2), .QN());

  wire const0, w, w2;

  AND2_X1 and_gate (.A1(d), .A2(d2), .ZN(w));
  LOGIC0_X1 const0_gate (.Z(const0));
  // data_check endpoint on A1, cell with a tied-off input
  OR2_X1 or_gate (.A1(w), .A2(const0), .ZN(w2));
  DFF_X1 flop3 (.CK(clk), .D(w2), .Q(q3), .QN());

  wire w3, w4;

  // data_check endpoint on A1, cell driving logic towards endpoint on DFF
  OR2_X1 or_gate2 (.A1(d), .A2(d2), .ZN(w3));
  BUF_X1 or_gate2_buf (.A(w3), .Z(w4));
  DFF_X1 flop4 (.CK(clk), .D(w4), .Q(q4), .QN());

endmodule
