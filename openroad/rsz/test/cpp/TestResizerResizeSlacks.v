// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
module top(input a, input unused, input loose_in,
           output y, output mixed_out, output loose_out);
  assign y = a;
  assign mixed_out = a;
  assign loose_out = loose_in;
endmodule
