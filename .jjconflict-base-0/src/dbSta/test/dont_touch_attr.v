module top (in1, out1, out2);
   input in1;
   output out1;
   output out2;

   (* dont_touch *) BUF_X1 u1 (.A(in1), .Z(out1));
   BUF_X1 u2 (.A(in1), .Z(out2));
endmodule // top
