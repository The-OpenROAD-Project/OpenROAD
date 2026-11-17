module top (load_output);
 output load_output;

 wire net;
 wire net1;

 BUF_X4 buf1 (.A(net),
    .Z(net1));
 LOGIC0_X1 drvr_inst (.Z(net));
 BUF_X1 load0_inst (.A(net1));
 BUF_X1 load2_inst (.A(net));
 MOD0 mi0 (.A(net));
 assign load_output = net;
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;

 wire net2;

 BUF_X4 buf2 (.A(A),
    .Z(net2));
 BUF_X1 load1_inst (.A(net2));
endmodule
