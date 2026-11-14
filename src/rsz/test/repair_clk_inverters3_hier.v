module regs (ck, d, q1, q2);
 input ck;
 input d;
 output q1;
 output q2;
 DFF_X1 r1 (.D(d), .CK(ck), .Q(q1));
 DFF_X1 r2 (.D(d), .CK(ck), .Q(q2));
endmodule

module top (clk, d, q1, q2, q3, q4);
 input clk;
 input d;
 output q1;
 output q2;
 output q3;
 output q4;
 wire clk_n;
 INV_X1 c1 (.A(clk), .ZN(clk_n));
 regs u_a (.ck(clk_n), .d(d), .q1(q1), .q2(q2));
 regs u_b (.ck(clk_n), .d(d), .q1(q3), .q2(q4));
endmodule
