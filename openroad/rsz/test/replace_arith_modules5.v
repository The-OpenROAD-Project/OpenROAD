// u_alu: an arithmetic module whose X outputs come back in through ports
// added by hierarchical buffering: loopX0 takes X[0] straight back, and
// loopX1 takes X[1] back through loopbuf_x1, which drives nothing else. The
// swap target (KOGGE_STONE) lacks both ports and drives its cells from X
// itself.
// u_alu_out: g0 leaves through an extra output port, which the swap target
// lacks, before coming back in through g0in, so the swap is refused.
module top (clk, a, b, bi, ci, x0, y, z);
  input clk;
  input [1:0] a;
  input [1:0] b;
  input bi;
  input ci;
  output x0;
  output [1:0] y;
  output [1:0] z;

  wire [1:0] aq;
  wire [1:0] bq;
  wire [1:0] x;
  wire x1_buf;
  wire [1:0] sum;
  wire g0;
  wire g0_buf;
  wire [1:0] sum_out;

  DFF_X1 ra0 (.D(a[0]), .CK(clk), .Q(aq[0]));
  DFF_X1 ra1 (.D(a[1]), .CK(clk), .Q(aq[1]));
  DFF_X1 rb0 (.D(b[0]), .CK(clk), .Q(bq[0]));
  DFF_X1 rb1 (.D(b[1]), .CK(clk), .Q(bq[1]));

  BUF_X1 loopbuf_x1 (.A(x[1]), .Z(x1_buf));
  DFF_X1 rx0 (.D(x[0]), .CK(clk), .Q(x0));

  ALU_2_0_2_0_2_unused_CO_HAN_CARLSON u_alu (
      .A(aq),
      .B(bq),
      .BI(bi),
      .CI(ci),
      .X(x),
      .Y(sum),
      .loopX0(x[0]),
      .loopX1(x1_buf)
  );

  BUF_X1 loopbuf_g0 (.A(g0), .Z(g0_buf));

  ALU_2_1_2_1_2_unused_CO_X_HAN_CARLSON u_alu_out (
      .A(aq),
      .B(bq),
      .BI(bi),
      .CI(ci),
      .Y(sum_out),
      .g0out(g0),
      .g0in(g0_buf)
  );

  DFF_X1 ry0 (.D(sum[0]), .CK(clk), .Q(y[0]));
  DFF_X1 ry1 (.D(sum[1]), .CK(clk), .Q(y[1]));
  DFF_X1 rz0 (.D(sum_out[0]), .CK(clk), .Q(z[0]));
  DFF_X1 rz1 (.D(sum_out[1]), .CK(clk), .Q(z[1]));
endmodule

(* arithmetic_operator = 1 *)
(* implements_operator = "ALU_2_0_2_0_2_unused_CO" *)
(* architecture = "HAN_CARLSON" *)
module ALU_2_0_2_0_2_unused_CO_HAN_CARLSON (A, B, BI, CI, X, Y, loopX0, loopX1);
  input [1:0] A;
  input [1:0] B;
  input BI;
  input CI;
  output [1:0] X;
  output [1:0] Y;
  input loopX0;
  input loopX1;

  wire g0;

  XOR2_X1 p0 (.A(A[0]), .B(B[0]), .Z(X[0]));
  XOR2_X1 p1 (.A(A[1]), .B(B[1]), .Z(X[1]));
  AND2_X1 c0 (.A1(A[0]), .A2(B[0]), .ZN(g0));
  XOR2_X1 s0 (.A(loopX0), .B(CI), .Z(Y[0]));
  XOR2_X1 s1 (.A(loopX1), .B(g0), .Z(Y[1]));
endmodule

(* arithmetic_operator = 1 *)
(* implements_operator = "ALU_2_0_2_0_2_unused_CO" *)
(* architecture = "KOGGE_STONE" *)
module ALU_2_0_2_0_2_unused_CO_KOGGE_STONE (A, B, BI, CI, X, Y);
  input [1:0] A;
  input [1:0] B;
  input BI;
  input CI;
  output [1:0] X;
  output [1:0] Y;

  wire g0;

  XOR2_X1 p0 (.A(A[0]), .B(B[0]), .Z(X[0]));
  XOR2_X1 p1 (.A(A[1]), .B(B[1]), .Z(X[1]));
  AND2_X1 c0 (.A1(A[0]), .A2(B[0]), .ZN(g0));
  XOR2_X1 s0 (.A(X[0]), .B(CI), .Z(Y[0]));
  XOR2_X1 s1 (.A(X[1]), .B(g0), .Z(Y[1]));
endmodule

(* arithmetic_operator = 1 *)
(* implements_operator = "ALU_2_1_2_1_2_unused_CO_X" *)
(* architecture = "HAN_CARLSON" *)
module ALU_2_1_2_1_2_unused_CO_X_HAN_CARLSON (A, B, BI, CI, Y, g0out, g0in);
  input [1:0] A;
  input [1:0] B;
  input BI;
  input CI;
  output [1:0] Y;
  output g0out;
  input g0in;

  XOR2_X1 s0 (.A(A[0]), .B(B[0]), .Z(Y[0]));
  AND2_X1 c0 (.A1(A[0]), .A2(B[0]), .ZN(g0out));
  XOR2_X1 s1 (.A(A[1]), .B(g0in), .Z(Y[1]));
endmodule

(* arithmetic_operator = 1 *)
(* implements_operator = "ALU_2_1_2_1_2_unused_CO_X" *)
(* architecture = "KOGGE_STONE" *)
module ALU_2_1_2_1_2_unused_CO_X_KOGGE_STONE (A, B, BI, CI, Y);
  input [1:0] A;
  input [1:0] B;
  input BI;
  input CI;
  output [1:0] Y;

  wire g0;

  XOR2_X1 s0 (.A(A[0]), .B(B[0]), .Z(Y[0]));
  AND2_X1 c0 (.A1(A[0]), .A2(B[0]), .ZN(g0));
  XOR2_X1 s1 (.A(A[1]), .B(g0), .Z(Y[1]));
endmodule
