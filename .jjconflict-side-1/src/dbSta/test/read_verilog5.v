module top (in1, in2, out1);
   input in1, in2;
   output out1;
   tri in1;

   bidir b1(.IO(in1), .A(in2), .Z(out1));
endmodule
