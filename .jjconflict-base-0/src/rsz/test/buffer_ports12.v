module top (
  input clk,
  input sram0_ce_in,
  input sram0_we_in,
  input [5:0] sram0_addr_in,
  input [6:0] sram0_w_mask_in,
  input sram1_ce_in,
  input sram1_we_in,
  input [5:0] sram1_addr_in,
  input [6:0] sram1_w_mask_in
);

  wire [6:0] sram0_rd_out;
  wire [6:0] sram1_rd_out;

  fakeram45_64x7 sram0 (
    .clk(clk),
    .ce_in(sram0_ce_in),
    .we_in(sram0_we_in),
    .addr_in(sram0_addr_in),
    .wd_in(sram1_rd_out),
    .w_mask_in(sram0_w_mask_in),
    .rd_out(sram0_rd_out)
  );

  fakeram45_64x7 sram1 (
    .clk(clk),
    .ce_in(sram1_ce_in),
    .we_in(sram1_we_in),
    .addr_in(sram1_addr_in),
    .wd_in(sram0_rd_out),
    .w_mask_in(sram1_w_mask_in),
    .rd_out(sram1_rd_out)
  );

endmodule
