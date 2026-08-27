// TOP: top
// TECH: nangate45
// TARGETS: concat_const, assign_const_output_port
// CLUE: output bus driven by an assign of a concat mixing constants and a
// signal: y = {2'b10, a, 1'b1} — const bits and live bit interleaved on the
// port driver.
module top (input a, output [3:0] y);
  assign y = {2'b10, a, 1'b1};
endmodule
