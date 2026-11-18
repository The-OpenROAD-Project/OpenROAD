module top ();

 wire net1;
 wire n1;

 BUF_X1 load0 (.A(n1));
 BUF_X1 load1 (.A(n1));
 BUF_X1 load4 (.A(net1));
 BUF_X1 new_buf1 (.A(n1),
    .Z(net1));
 ModH0 u_h0 (.out(n1));
 ModH1 u_h1 (.in(net1));
 ModH2 u_h2 (.in(net1));
endmodule
module ModH0 (out);
 output out;


 LOGIC0_X1 drvr (.Z(out));
endmodule
module ModH1 (in);
 input in;


 BUF_X1 load2 (.A(in));
endmodule
module ModH2 (in);
 input in;


 ModH3 u_h3 (.in(in));
endmodule
module ModH3 (in);
 input in;


 BUF_X1 load3 (.A(in));
endmodule
