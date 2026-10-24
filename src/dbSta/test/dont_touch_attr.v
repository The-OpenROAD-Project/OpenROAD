module top (in1, out1, out2, out3, out4, out5, out6, out7, out8);
   input in1;
   output out1;
   output out2;
   output out3;
   output out4;
   output out5;
   output out6;
   output out7;
   output out8;

   (* dont_touch *) BUF_X1 u1 (.A(in1), .Z(out1));
   BUF_X1 u2 (.A(in1), .Z(out2));
   (* dont_touch = "true" *) BUF_X1 u3 (.A(in1), .Z(out3));
   (* dont_touch = "yes" *) BUF_X1 u4 (.A(in1), .Z(out4));
   (* dont_touch = "false" *) BUF_X1 u5 (.A(in1), .Z(out5));
   (* dont_touch = "maybe" *) BUF_X1 u6 (.A(in1), .Z(out6));
   (* dont_touch = "True" *) BUF_X1 u7 (.A(in1), .Z(out7));
   (* dont_touch = "1" *) BUF_X1 u8 (.A(in1), .Z(out8));
endmodule // top
