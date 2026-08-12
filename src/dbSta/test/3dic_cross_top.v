// Stub flop_chip_a / flop_chip_b modules so STA's verilog reader doesn't
// black-box them when buildChipNetsFromVerilog parses this file. The real
// bodies are loaded separately from each chiplet's DEF.
module flop_chip_a (clk, d, q);
  input clk;
  input d;
  output q;
endmodule

module flop_chip_b (clk, d, q);
  input clk;
  input d;
  output q;
endmodule

module top (clk_top, in_top, out_top);
  input clk_top;
  input in_top;
  output out_top;
  wire bridge;
  flop_chip_a chipA (.clk(clk_top), .d(in_top), .q(bridge));
  flop_chip_b chipB (.clk(clk_top), .d(bridge), .q(out_top));
endmodule
