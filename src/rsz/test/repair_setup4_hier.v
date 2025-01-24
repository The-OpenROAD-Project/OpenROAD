/*
 Hierarchical version of repair_setup4_flat.v
 */


module reg1 (clk);
 input clk;

 DFF_X1 r1 (.CK(clk),
    .Q(r1q));
   
   submodule u1(.r1q(r1q),
		.clk(clk)
		);
   
endmodule // reg1

module submodule(input r1q,
		 input clk)   ;

 wire u1z;
 wire u2z;
 wire u3z;
 wire u4z;
 wire u5z;
   
 BUF_X1 u1 (.A(r1q),
    .Z(u1z));
 BUF_X1 u2 (.A(u1z),
    .Z(u2z));
 BUF_X1 u3 (.A(u2z),
    .Z(u3z));
 BUF_X1 u4 (.A(u3z),
    .Z(u4z));
 BUF_X1 u5 (.A(u4z),
    .Z(u5z));
 DFF_X1 r2 (.D(u5z),
    .CK(clk));
 DFF_X1 r3 (.D(r1q),
    .CK(clk));
 DFF_X1 r4 (.D(r1q),
    .CK(clk));
 DFF_X1 r5 (.D(r1q),
    .CK(clk));
 DFF_X1 r6 (.D(r1q),
    .CK(clk));
 DFF_X1 r7 (.D(r1q),
    .CK(clk));
 DFF_X1 r8 (.D(r1q));
 DFF_X1 r9 (.D(r1q));
 DFF_X1 r10 (.D(r1q));
 DFF_X1 r11 (.D(r1q));
 DFF_X1 r12 (.D(r1q));
endmodule
