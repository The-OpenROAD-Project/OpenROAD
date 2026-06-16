module top ();

 wire w0;
 wire w1;
 wire mid;

 block b (.i(mid),
    .o({w1,
    w0}));
 INV_X1 leaf (.A(w0),
    .ZN(mid));
 INV_X1 load0 (.A(w0));
 INV_X1 load1 (.A(w1));
endmodule
module block (i,
    o);
 input i;
 output [1:0] o;

 wire net1;
 wire net2;

 LOGIC0_X1 buf0_1 (.Z(net1));
 INV_X1 leaf (.A(net2),
    .ZN(o[0]));
 LOGIC0_X1 leaf_2 (.Z(net2));
 assign o[1] = net1;
endmodule
