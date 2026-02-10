module top (in);
 input in;

 wire n1;

 BUF_X1 drvr (.A(in),
    .Z(n1));
 BUF_X1 load0 (.A(n1));
 H0 h0 (.h0_in(n1));
endmodule
module H0 (h0_in);
 input h0_in;

 BUF_X1 load1 (.A(h0_in));
endmodule
