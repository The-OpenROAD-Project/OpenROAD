// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
module top(input clk, input a, output y0, output y1);
  wire q0, q1, d0, d1;
  DFF_X1 launch0 (.CK(clk), .D(a), .Q(q0));
  DFF_X1 launch1 (.CK(clk), .D(a), .Q(q1));
  BUF_X1 buffer0 (.A(q0), .Z(d0));
  BUF_X1 buffer1 (.A(q1), .Z(d1));
  DFF_X1 capture0 (.CK(clk), .D(d0), .Q(y0));
  DFF_X1 capture1 (.CK(clk), .D(d1), .Q(y1));
endmodule
