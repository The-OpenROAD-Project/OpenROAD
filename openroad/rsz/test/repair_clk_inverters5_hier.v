module cgen (clk, clk_n_out);
 input clk;
 output clk_n_out;
 INV_X1 c1 (.A(clk), .ZN(clk_n_out));
endmodule

module regs (ck, d, q1, q2);
 input ck;
 input d;
 output q1;
 output q2;
 DFF_X1 r1 (.D(d), .CK(ck), .Q(q1));
 DFF_X1 r2 (.D(d), .CK(ck), .Q(q2));
endmodule

module top (clk, d, q1, q2);
 input clk;
 input d;
 output q1;
 output q2;
 wire clk_n;
 cgen u_cg (.clk(clk), .clk_n_out(clk_n));
 regs u_regs (.ck(clk_n), .d(d), .q1(q1), .q2(q2));
endmodule
