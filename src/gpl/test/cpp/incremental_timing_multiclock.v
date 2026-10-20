// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
module top(input clk, input clk_other, input a, input async_in,
           output y0, output y1, output async_out);
  wire q0, q1, d0, d1, async_net;
  DFF_X1 launch0 (.CK(clk), .D(a), .Q(q0));
  DFF_X1 launch1 (.CK(clk_other), .D(a), .Q(q1));
  BUF_X1 buffer0 (.A(q0), .Z(d0));
  BUF_X1 buffer1 (.A(q1), .Z(d1));
  DFF_X1 capture0 (.CK(clk), .D(d0), .Q(y0));
  DFF_X1 capture1 (.CK(clk_other), .D(d1), .Q(y1));
  BUF_X1 unconstrained_buffer (.A(async_in), .Z(async_net));
  BUF_X1 unconstrained_load (.A(async_net), .Z(async_out));
endmodule
