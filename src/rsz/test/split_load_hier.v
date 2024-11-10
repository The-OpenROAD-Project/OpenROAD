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
 wire u6z;
 wire u7z;
 wire u8z;      
 wire u9z;      
 wire u10z;      
 wire u11z;      
 wire u12z;      
 wire u13z;      
 wire u14z;      
 wire u15z;      
 wire u16z;      
 wire u17z;      
 wire u18z;      
 wire u19z;      
 wire u20z;      

 BUF_X1 u1 (.A(r1q),
    .Z(u6z));
 DFF_X1 r1 (.D(u6z),
    .CK(clk));

 BUF_X1 u2 (.A(r1q),
    .Z(u7z));
 DFF_X1 r2 (.D(u7z),
    .CK(clk));

 BUF_X1 u3 (.A(r1q),
    .Z(u8z));
 DFF_X1 r3 (.D(u8z),
    .CK(clk));
   
 BUF_X1 u4 (.A(r1q),
    .Z(u9z));
 DFF_X1 r4 (.D(u9z),
    .CK(clk));

   
 BUF_X1 u5 (.A(r1q),
    .Z(u10z));
 DFF_X1 r5 (.D(u10z),
    .CK(clk));
   
 BUF_X1 u6 (.A(r1q),
    .Z(u11z));
 DFF_X1 r6 (.D(u11z),
    .CK(clk));
   
 BUF_X1 u7 (.A(r1q),
    .Z(u12z));
 DFF_X1 r7 (.D(u12z),
    .CK(clk));
   
 BUF_X1 u8 (.A(r1q),
    .Z(u13z));
 DFF_X1 r8 (.D(u13z),
    .CK(clk));
   
 BUF_X1 u9 (.A(r1q),
    .Z(u14z));
 DFF_X1 r9 (.D(u14z),
    .CK(clk));
   
 BUF_X1 u10 (.A(r1q),
    .Z(u15z));
 DFF_X1 r10 (.D(u15z),
    .CK(clk));
   
 BUF_X1 u11 (.A(r1q),
    .Z(u16z));
 DFF_X1 r11 (.D(u16z),
    .CK(clk));
   
 BUF_X1 u12 (.A(r1q),
    .Z(u17z));
 DFF_X1 r12 (.D(u17z),
    .CK(clk));
   
 BUF_X1 u13 (.A(r1q),
    .Z(u18z));
 DFF_X1 r13 (.D(u18z),
    .CK(clk));
   
 BUF_X1 u14 (.A(r1q),
    .Z(u19z));
 DFF_X1 r14 (.D(u19z),
    .CK(clk));
   
 BUF_X1 u15 (.A(r1q),
    .Z(u20z));
 DFF_X1 r15 (.D(u20z),
    .CK(clk));
   
   
 BUF_X1 u16 (.A(r1q),
    .Z(u1z));
 BUF_X1 u17 (.A(u1z),
    .Z(u2z));
 BUF_X1 u18 (.A(u2z),
    .Z(u3z));
 BUF_X1 u19 (.A(u3z),
    .Z(u4z));
 BUF_X1 u20 (.A(u4z),
    .Z(u5z));
 DFF_X1 r21 (.D(u5z),
    .CK(clk));
 DFF_X1 r22 (.D(r1q),
    .CK(clk));
 DFF_X1 r23 (.D(r1q),
    .CK(clk));
 DFF_X1 r24 (.D(r1q),
    .CK(clk));
 DFF_X1 r25 (.D(r1q),
    .CK(clk));
 DFF_X1 r26 (.D(r1q),
    .CK(clk));
 DFF_X1 r27 (.D(r1q));
 DFF_X1 r28 (.D(r1q));
 DFF_X1 r29 (.D(r1q));
 DFF_X1 r30 (.D(r1q));
 DFF_X1 r31 (.D(r1q));
endmodule
