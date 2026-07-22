module SoC (
  input clk,
  input [7:0] data_in,
  output [7:0] data_out
);
  assign data_out = data_in;
endmodule
