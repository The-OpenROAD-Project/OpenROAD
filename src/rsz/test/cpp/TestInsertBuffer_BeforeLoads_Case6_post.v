module top ();

 wire net1;
 wire n1;

 BUF_X1 load0 (.A(n1));
 BUF_X1 new_buf1 (.A(n1),
    .Z(net1));
 ModH2 u_h2 (.n1(net1),
    .out(n1));
 ModH3 u_h3 (.n1(net1),
    .in(n1));
 ModH4 u_h4 (.n1(net1),
    .in(n1));
endmodule
module ModH0 (out);
 output out;


 LOGIC0_X1 drvr (.Z(out));
endmodule
module ModH1 (in);
 input in;


 BUF_X1 load5 (.A(in));
endmodule
module ModH2 (n1,
    out);
 input n1;
 output out;

 wire n_internal;

 ModH0 u_h0 (.out(n_internal));
 ModH1 u_h1 (.in(n1));
 assign out = n_internal;
endmodule
module ModH3 (n1,
    in);
 input n1;
 input in;


 BUF_X1 load1 (.A(in));
 BUF_X1 load2 (.A(n1));
endmodule
module ModH4 (n1,
    in);
 input n1;
 input in;


 BUF_X1 load4 (.A(in));
 ModH5 u_h5 (.in(n1));
endmodule
module ModH5 (in);
 input in;


 BUF_X1 load3 (.A(in));
endmodule
