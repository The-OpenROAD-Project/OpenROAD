module memory_nangate45(
    input  wire clk_i,
    input  wire we_i,
    input  wire ce_i,
    input  wire [5:0] addr_i,
    input  wire [6:0] d_i,
    input  wire [6:0] wmask_i,
    output wire [6:0] d_o
);

    fakeram45_64x7 i_memory (
        .clk(clk_i),
        .we_in(we_i),
        .ce_in(ce_i),
        .addr_in(addr_i),
        .wd_in(d_i),
        .w_mask_in(wmask_i),
        .rd_out(d_o)
    );

endmodule
