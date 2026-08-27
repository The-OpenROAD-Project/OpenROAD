// TARGETS: perm_swap, sequential, concat_port_conn, depth_2
// CLUE: The bit swap sits on the port connection that feeds a bank of flops one
// CLUE: level down, so the wrong binding shows up only after a clock edge. One
// CLUE: flop is read from Q and the other from QN so the two registered bits
// CLUE: are different functions of different inputs.

module top (clk, d, q);
 input clk;
 input [1:0] d;
 output [1:0] q;
 mid u (.ck(clk), .din({d[0],d[1]}), .dout(q));
endmodule

module mid (ck, din, dout);
 input ck;
 input [1:0] din;
 output [1:0] dout;
 regs u (.ck(ck), .din(din), .dout(dout));
endmodule

module regs (ck, din, dout);
 input ck;
 input [1:0] din;
 output [1:0] dout;
 DFF_X1 f0 (.D(din[0]), .CK(ck), .Q(dout[0]));
 DFF_X1 f1 (.D(din[1]), .CK(ck), .QN(dout[1]));
endmodule
