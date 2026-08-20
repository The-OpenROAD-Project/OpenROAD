// TOP: top
// TECH: nangate45
// ORIGIN: ../../../../rsz/test/cpp/TestBufferRemoval3_1.v
// RECLAIMED: renamed instance 'buf' to 'ibuf'
// Was quarantined because: names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
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
 BUF_X2 ibuf (.A(net1),
    .Z(net2));
 BUF_X4 load (.A(net2),
    .Z(out1));
 SUBMOD sub_inst (.in(net2),
    .out(out2));
endmodule

module SUBMOD (in,
    out);
 input in;
 output out;

 BUF_X1 load0 (.A(in), .Z(out));
endmodule
