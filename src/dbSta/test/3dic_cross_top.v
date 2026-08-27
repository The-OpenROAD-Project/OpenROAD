// Stub flop_chip_a / flop_chip_b modules so STA's verilog reader doesn't
// black-box them when buildChipNetsFromVerilog parses this file. The real
// bodies are loaded separately from each chiplet's DEF.
// flop_chip_a's raw port has no bump map entry: it connects to the top
// through its bterm alone, covering the bump-less connection path.
module flop_chip_a (clk, d, q, raw);
  input clk;
  input d;
  output q;
  output raw;
endmodule

module flop_chip_b (clk, d, q);
  input clk;
  input d;
  output q;
endmodule

module top (clk_top, in_top, out_top, raw_top);
  input clk_top;
  input in_top;
  output out_top;
  output raw_top;
  wire bridge;
  flop_chip_a chipA (.clk(clk_top), .d(in_top), .q(bridge), .raw(raw_top));
  flop_chip_b chipB (.clk(clk_top), .d(bridge), .q(out_top));
endmodule
