module top (in,
    out);
 input in;
 output out;

 wire n3;
 wire n4;
 wire n2;
 wire n1;

 H0 h0 (.h0_in(in),
    .h0_out(n1),
    .h0_out2(n2));
 H1 h1 (.h1_in(n2),
    .h1_in1(n4));
 H3 h3 (.h3_in(n1),
    .h3_out(n3),
    .h3_out1(n4),
    .h3_out2(out));
 BUF_X1 nontarget0 (.A(n3));
endmodule
module H0 (h0_in,
    h0_out,
    h0_out2);
 input h0_in;
 output h0_out;
 output h0_out2;


 BUF_X1 buf0 (.A(h0_out),
    .Z(h0_out2));
 BUF_X1 drvr (.A(h0_in),
    .Z(h0_out));
endmodule
module H1 (h1_in,
    h1_in1);
 input h1_in;
 input h1_in1;


 H2 h2 (.h2_in(h1_in));
endmodule
module H2 (h2_in);
 input h2_in;


 BUF_X1 load0 (.A(h2_in));
endmodule
module H3 (h3_in,
    h3_out,
    h3_out1,
    h3_out2);
 input h3_in;
 output h3_out;
 output h3_out1;
 output h3_out2;


 // jk: TODO: this should be supported
 //assign h3_out = h3_in;
 //assign h3_out1 = h3_in;
 //assign h3_out2 = h3_in;
 BUF_X1 assign0 (.A(h3_in),
    .Z(h3_out));
 BUF_X1 assign1 (.A(h3_in),
    .Z(h3_out1));
 BUF_X1 assign2 (.A(h3_in),
    .Z(h3_out2));
 BUF_X1 load1 (.A(h3_in));
 BUF_X1 load2 (.A(h3_in));
endmodule
