// TOP: top
// TECH: nangate45
// ORIGIN: ../../../../rsz/test/cpp/TestInsertBuffer_BeforeLoads_Case17_post.v
// RECLAIMED: renamed instance 'buf' to 'ibuf'; exposed dangling nets as ports: n2
// Was quarantined because: names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
module top (
    in,
    n2
);
 output n2;

 input in;

 wire net1;
 wire n2;
 wire n1;

 BUF_X1 ibuf (.A(n1),
    .Z(n2));
 BUF_X1 drvr (.A(in),
    .Z(n1));
 H0 h0 (.h0_in(net1));
 BUF_X1 load0 (.A(net1));
 BUF_X4 new_buf1 (.A(n1),
    .Z(net1));
 BUF_X1 non_target0 (.A(n1));
endmodule
module H0 (h0_in);
 input h0_in;


 BUF_X1 load1 (.A(h0_in));
endmodule
