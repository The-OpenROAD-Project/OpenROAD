module regs (ck, d, q1, q2);
 input ck;
 input d;
 output q1;
 output q2;
 DFF_X1 r1 (.D(d), .CK(ck), .Q(q1));
 DFF_X1 r2 (.D(d), .CK(ck), .Q(q2));
endmodule

module mid (clk_in, d, clk_n_out, q1, q2);
 input clk_in;
 input d;
 output clk_n_out;
 output q1;
 output q2;
 INV_X1 c1 (.A(clk_in), .ZN(clk_n_out));
 regs u_regs (.ck(clk_n_out), .d(d), .q1(q1), .q2(q2));
endmodule

module top (clk, d, clk_n_o, q1, q2);
 input clk;
 input d;
 output clk_n_o;
 output q1;
 output q2;
 mid m1 (.clk_in(clk), .d(d), .clk_n_out(clk_n_o), .q1(q1), .q2(q2));
endmodule
