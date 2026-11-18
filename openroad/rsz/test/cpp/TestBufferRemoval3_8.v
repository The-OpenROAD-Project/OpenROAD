// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024, The OpenROAD Authors

module top (
  input clk,
  input in,
  output out
);

  BUF_X1 buf1 (.Z(out));
endmodule
