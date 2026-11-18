module top (clk,
    in1,
    in2,
    out);
 input clk;
 input in1;
 input in2;
 output out;

 wire i1z;
 wire i2z;
 wire i3z;
 wire i4z;
 wire i5z;
 wire i6z;
 wire i7z;
 wire i8z;
 wire r1q;
 wire r2q;
 wire u1z;
 wire u2z;

 BUF_X1 i1 (.A(clk),
    .Z(i1z));
 BUF_X1 i2 (.A(i1z),
    .Z(i2z));
 BUF_X1 i3 (.A(i2z),
    .Z(i3z));
 BUF_X1 i4 (.A(i3z),
    .Z(i4z));
 BUF_X1 i5 (.A(i4z),
    .Z(i5z));
 BUF_X1 i6 (.A(i5z),
    .Z(i6z));
 BUF_X1 i7 (.A(i6z),
    .Z(i7z));
 BUF_X1 i8 (.A(i7z),
    .Z(i8z));
   
 hier1 h1 (.in1(in1),
    .i1z(i1z),
    .op(r1q));
   
 DFF_X1 r2 (.D(in2),
    .CK(i4z),
    .Q(r2q));
 DFF_X1 r3 (.D(u2z),
    .CK(i8z),
    .Q(out));
 BUF_X1 u1 (.A(r2q),
    .Z(u1z));
 AND2_X1 u2 (.A1(r1q),
    .A2(u1z),
    .ZN(u2z));
endmodule // top

module hier1(in1,i1z,op);

   input in1;
   input i1z;
   output op;
   DFF_X1 r1 (.D(in1),
    .CK(i1z),
    .Q(op));
endmodule // hier1


