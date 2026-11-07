// r1 -> buffer chain -> r2 sets the real clock's min period.
// in2 -> f1 -> out2 is the only path timed by the virtual clock alone.
module top (clk, in1, in2, out1, out2);
  input clk;
  input in1;
  input in2;
  output out1;
  output out2;

  wire q1;
  wire n1;
  wire n2;
  wire n3;
  wire n4;
  wire n5;
  wire n6;
  wire n7;
  wire n8;

  DFF_X1 r1 (.D(in1), .CK(clk), .Q(q1));
  BUF_X1 b1 (.A(q1), .Z(n1));
  BUF_X1 b2 (.A(n1), .Z(n2));
  BUF_X1 b3 (.A(n2), .Z(n3));
  BUF_X1 b4 (.A(n3), .Z(n4));
  BUF_X1 b5 (.A(n4), .Z(n5));
  BUF_X1 b6 (.A(n5), .Z(n6));
  BUF_X1 b7 (.A(n6), .Z(n7));
  BUF_X1 b8 (.A(n7), .Z(n8));
  DFF_X1 r2 (.D(n8), .CK(clk), .Q(out1));
  BUF_X1 f1 (.A(in2), .Z(out2));
endmodule
