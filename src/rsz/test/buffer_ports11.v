module top (
  input clk,
  input ce_in,
  input we_in,
  input [5:0] addr_in,
  input [6:0] wd_in,
  input [6:0] w_mask_in,
  output [6:0] rd_out
);

  fakeram45_64x7 sram (
    .clk(clk),
    .ce_in(ce_in),
    .we_in(we_in),
    .addr_in(addr_in),
    .wd_in(wd_in),
    .w_mask_in(w_mask_in),
    .rd_out(rd_out)
  );

endmodule
