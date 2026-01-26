module top (in);
 input in;

 wire n2;
 wire n1;

 BUF_X1 buf (.A(n1),
    .Z(n2));
 BUF_X1 drvr (.A(in),
    .Z(n1));
 H0 h0 (.h0_in(n2));
 BUF_X1 load0 (.A(n1));
 BUF_X1 non_target0 (.A(n1));
endmodule
module H0 (h0_in);
 input h0_in;

 BUF_X1 load1 (.A(h0_in));
endmodule
