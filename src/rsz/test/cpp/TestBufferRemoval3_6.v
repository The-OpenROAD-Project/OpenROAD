// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024, The OpenROAD Authors

module top (
  input clk,
  input in,
  output out
);
  MOD mod_inst (.mod_in(in), .mod_out(out));
endmodule

module MOD (input mod_in, output mod_out);
  BUF_X1 buf1 (.A(mod_in), .Z(mod_out));
endmodule