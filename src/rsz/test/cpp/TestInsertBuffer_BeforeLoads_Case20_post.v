module top (in,
    rd_out);
 input in;
 output [6:0] rd_out;

 wire net1;
 wire n1;

 BUF_X1 drvr (.A(in),
    .Z(n1));
 H0 h0 (.n1(net1),
    .h0_in(n1));
 H2 h2 (.h2_in(net1),
    .h2_rd_out({rd_out[6],
    rd_out[5],
    rd_out[4],
    rd_out[3],
    rd_out[2],
    rd_out[1],
    rd_out[0]}));
 BUF_X4 new_buf1 (.A(n1),
    .Z(net1));
endmodule
module H0 (n1,
    h0_in);
 input n1;
 input h0_in;


 H1 h1 (.n1(n1),
    .h1_in(h0_in));
 BUF_X1 load0 (.A(n1));
endmodule
module H1 (n1,
    h1_in);
 input n1;
 input h1_in;


 BUF_X1 load1 (.A(n1));
 BUF_X1 nontarget0 (.A(h1_in));
 BUF_X1 nontarget1 (.A(h1_in));
 BUF_X1 nontarget2 (.A(h1_in));
endmodule
module H2 (h2_in,
    h2_rd_out);
 input h2_in;
 output [6:0] h2_rd_out;

 wire h2_tie;

 LOGIC0_X1 h2_tie0 (.Z(h2_tie));
 fakeram45_64x7 mem (.we_in(h2_in),
    .ce_in(h2_tie),
    .clk(h2_tie),
    .addr_in({h2_tie,
    h2_tie,
    h2_tie,
    h2_tie,
    h2_tie,
    h2_tie}),
    .rd_out({h2_rd_out[6],
    h2_rd_out[5],
    h2_rd_out[4],
    h2_rd_out[3],
    h2_rd_out[2],
    h2_rd_out[1],
    h2_rd_out[0]}),
    .w_mask_in({h2_tie,
    h2_tie,
    h2_tie,
    h2_tie,
    h2_tie,
    h2_tie,
    h2_tie}),
    .wd_in({h2_tie,
    h2_tie,
    h2_tie,
    h2_tie,
    h2_tie,
    h2_tie,
    h2_tie}));
endmodule
