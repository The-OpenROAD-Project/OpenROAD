module top ();

 wire drvr_net;
 wire net;

 BUF_X1 buf (.A(drvr_net),
    .Z(net));
 LOGIC0_X1 drvr_inst (.Z(drvr_net));
 BUF_X1 load1_inst (.A(net));
 BUF_X1 load2_inst (.A(net));
endmodule
