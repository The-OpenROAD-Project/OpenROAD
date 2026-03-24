module top (in_term);
 input in_term;

 wire net1;
 wire net_orig;
 wire net_inter;

 BUF_X1 drvr (.A(in_term),
    .Z(net_orig));
 EXEC exec_inst (.in(net1));
 BUF_X4 new_buf1 (.A(net_orig),
    .Z(net1));
 WB wb_inst (.net_orig(net1),
    .in(net_orig),
    .out(net_inter));
endmodule
module EXEC (in);
 input in;


 BUF_X1 load_exec (.A(in));
endmodule
module WB (net_orig,
    in,
    out);
 input net_orig;
 input in;
 output out;


 BUF_X1 feedthru_buf (.A(in),
    .Z(out));
 BUF_X1 load_internal (.A(net_orig));
endmodule
