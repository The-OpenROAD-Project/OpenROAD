// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024, The OpenROAD Authors

module top (
  input clk,
  input in1,
  output out1
);

  wire dff_q;
  wire buf0_out;
  wire buf1_out;

  // D-FF
  DFF_X1 dff_inst (.D(in1), .CK(clk), .Q(dff_q));

  // buf0
  BUF_X1 buf0 (.A(dff_q), .Z(buf0_out));

  // mod
  MOD mod_inst (.in(buf0_out), .out(buf1_out));

  // buf2
  BUF_X1 buf2 (.A(buf1_out), .Z(out1));

endmodule

module MOD (input in, output out);
  BUF_X1 buf1 (.A(in), .Z(out));
endmodule
