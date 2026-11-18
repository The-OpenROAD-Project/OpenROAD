module top (in,
    rd_out);
 input in;
 output [6:0] rd_out;

 wire tie;
 wire n2;
 wire n1; 

 BUF_X1 buf (.A(n1),
    .Z(n2));
 BUF_X1 drvr (.A(in),
    .Z(n1));
 H0 h0 (.h0_in(n2),
    .h0_tie(tie),
    .h0_rd_out({rd_out[6],
    rd_out[5],
    rd_out[4],
    rd_out[3],
    rd_out[2],
    rd_out[1],
    rd_out[0]}));
 BUF_X1 load0 (.A(n1));
 BUF_X1 non_target0 (.A(n1));
 LOGIC0_X1 tie0 (.Z(tie));
endmodule
module H0 (h0_in,
    h0_tie,
    h0_rd_out);
 input h0_in;
 input h0_tie;
 output [6:0] h0_rd_out;


 fakeram45_64x7 mem (.we_in(h0_tie),
    .ce_in(h0_tie),
    .clk(h0_tie),
    .addr_in({h0_tie,
    h0_tie,
    h0_tie,
    h0_tie,
    h0_tie,
    h0_tie}),
    .rd_out(h0_rd_out),
    .w_mask_in({h0_tie,
    h0_tie,
    h0_tie,
    h0_tie,
    h0_in,
    h0_in,
    h0_in}),
    .wd_in({h0_tie,
    h0_tie,
    h0_tie,
    h0_tie,
    h0_tie,
    h0_tie,
    h0_tie}));
endmodule
