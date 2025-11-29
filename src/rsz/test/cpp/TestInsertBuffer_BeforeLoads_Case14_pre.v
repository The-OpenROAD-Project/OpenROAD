module top ();

 wire drvr_net;
 wire other_net;

 LOGIC0_X1 drvr_inst (.Z(drvr_net));
 BUF_X1 load1_inst (.A(drvr_net));
 BUF_X1 load2_inst (.A(other_net));
endmodule
