module top (clk,
    in1,
    out1,
    out2);
 input clk;
 input in1;
 output out1;
 output out2;

 wire net1;
 wire net2;

 BUF_X1 drvr (.A(in1),
    .Z(net1));
 BUF_X2 buf (.A(net1),
    .Z(net2));
 BUF_X4 load (.A(net2),
    .Z(out1));
 SUBMOD sub_inst (.mod_in(net2),
    .mod_out(out2));
endmodule

module SUBMOD (mod_in,
    mod_out);
 input mod_in;
 output mod_out;

 BUF_X1 load0 (.A(mod_in), .Z(mod_out));
endmodule
