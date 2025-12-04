module top (in_term);
 input in_term;

 wire net_orig;
 wire net_inter;

 BUF_X1 drvr (.A(in_term),
    .Z(net_orig));
 EXEC exec_inst (.in(net_inter));
 WB wb_inst (.in(net_orig),
    .out(net_inter));
endmodule
module EXEC (in);
 input in;


 BUF_X1 load_exec (.A(in));
endmodule
module WB (in,
    out);
 input in;
 output out;


 BUF_X1 feedthru_buf (.A(in),
    .Z(out));
 BUF_X1 load_internal (.A(in));
endmodule
