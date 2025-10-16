module top (clk);
 input clk;

 wire r1q;
 wire u1z;
 wire u2z;
 wire u3z;
 wire u4z;
 wire u5z;
 wire u6z;
 wire u7z;

 DFF_X1 r1 (.CK(clk),
    .Q(r1q));
 BUF_X1 u1 (.A(r1q),
    .Z(u1z));

 buffer_chain bc1 (.I(u1z), .O(u3z));
 buffer_chain bc2 (.I(u1z), .O(u6z));
 
 inv_chain ic1 (.I(u1z), .O(u5z));
 inv_chain ic2 (.I(u1z), .O(u7z));
   
 DFF_X1 r2 (.D(u3z),
    .CK(clk));
 DFF_X1 r3 (.D(u5z),
    .CK(clk));
 DFF_X1 r4 (.D(u6z),
    .CK(clk));
 DFF_X1 r5 (.D(u7z),
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


module buffer_chain (I, O);
   input I;
   output O;
 BUF_X1 u2 (.A(I),
    .Z(u2z));
 BUF_X1 u3 (.A(u2z),
    .Z(O));
endmodule // buffer_chain

module inv_chain (I, O);
   input I;
   output O;
 INV_X1 u4 (.A(I),
    .ZN(u4z));
 INV_X1 u5 (.A(u4z),
    .ZN(O));
endmodule // inv_chain


