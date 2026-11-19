module top (load_output);
 output load_output;

 wire net;

 LOGIC0_X1 drvr_inst (.Z(net));
 BUF_X1 load0_inst (.A(net));
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


 BUF_X1 load1_inst (.A(A));
endmodule
