// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024, The OpenROAD Authors

module top (
  input clk,
  input in1,
  output out1,
  output out2,
  output out3,
  output out4,
  output out5,
  output out6
);

  wire drvr_out;
  wire buf_top1_out;
  wire buf_top2_out;
  wire buf_mod2_out;

  // Level 1: Driver in a hierarchical module
  MOD1 mod1_inst (
    .clk_in(clk),
    .d_in(in1),
    .q_out(drvr_out)
  );

  // Level 2: drvr_out drives 2 top-level buffers and 1 hierarchical buffer
  BUF_X1 buf_top1 (.A(drvr_out), .Z(buf_top1_out));
  BUF_X1 buf_top2 (.A(drvr_out), .Z(buf_top2_out));
  MOD2 mod2_inst (.in(drvr_out), .out(buf_mod2_out));

  // Level 3: Loads driven by level 2 buffers
  // Loads of buf_top1
  BUF_X1 load_top1 (.A(buf_top1_out), .Z(out1));
  BUF_X1 load_top2 (.A(buf_top1_out), .Z(out2));

  // Loads of buf_top2
  BUF_X1 load_top3 (.A(buf_top2_out), .Z(out3));

  // Loads of buf_mod2 (from MOD2)
  BUF_X1 load_top4 (.A(buf_mod2_out), .Z(out4));

  // Hierarchical loads in MOD3
  MOD3 mod3_inst (
    .in1(buf_top1_out),
    .in2(buf_top2_out),
    .in3(buf_mod2_out),
    .out1(out5),
    .out2(out6)
  );

endmodule

module MOD1 (input clk_in, input d_in, output q_out);
  DFF_X1 drvr (.D(d_in), .CK(clk_in), .Q(q_out));
endmodule

module MOD2 (input in, output out);
  BUF_X1 buf_mod2 (.A(in), .Z(out));
endmodule

module MOD3 (input in1, input in2, input in3, output out1, output out2);
  BUF_X1 load_mod3_1 (.A(in1), .Z(out1));
  BUF_X1 load_mod3_2 (.A(in2), .Z(out2));
  BUF_X1 load_mod3_3 (.A(in3)); // This one is not connected to an output
endmodule