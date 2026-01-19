module top (in);
 input in;

 wire n2;

 H0 h0 (.h0_in(in),
    .h0_out(n2));
 H1 h1 (.h1_in(n2));
endmodule
module H0 (h0_in, 
    h0_out);
 input h0_in;
 output h0_out;

 wire n1;

 BUF_X1 drvr (.A(h0_in),
    .Z(n1));
 BUF_X1 nontarget0 (.A(n1));
 BUF_X1 buf0 (.A(n1),
    .Z(h0_out));
endmodule
module H1 (h1_in);
 input h1_in;

 wire n3;

 BUF_X1 buf1 (.A(h1_in),
    .Z(n3));
 BUF_X1 load0 (.A(n3));
 BUF_X1 load1 (.A(n3));
endmodule
