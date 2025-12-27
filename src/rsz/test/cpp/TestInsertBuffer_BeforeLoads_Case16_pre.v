module top (out);
 output out;

 wire net1;

 BUF_X1 load0 (.A(net1), .Z(out));
 BUF_X1 non_target0 (.A(net1));
 H0 h0 (.h0_out(net1));
endmodule
module H0 (h0_out);
 output h0_out;


 H1 h1(.h1_out(h0_out));
 BUF_X1 load1 (.A(h0_out));
endmodule
module H1 (h1_out);
 output h1_out;

 LOGIC0_X1 h0_h1_drvr (.Z(h1_out));
endmodule
