module top (clk,
    in1,
    out1);
 input clk;
 input in1;
 output out1;

 wire sub_out;

 BUF_X4 load (.A(sub_out),
    .Z(out1));
 SUBMOD sub_inst (.in(in1),
    .out(sub_out));
endmodule
module SUBMOD (in,
    out);
 input in;
 output out;

 wire net1;

 BUF_X2 buf (.A(in),
    .Z(net1));
 BUF_X1 load0 (.A(net1),
    .Z(out));
endmodule
