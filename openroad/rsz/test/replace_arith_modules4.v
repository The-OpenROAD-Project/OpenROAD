// u_alu: an arithmetic module with two ports added by hierarchical
// buffering. dupA0 and dupB1 carry buffered copies of A[0] and B[1], which
// also enter through the original ports. The swap target (KOGGE_STONE)
// lacks them. dupbuf_a0 drives only dupA0; dupbuf_b1 also drives a flop.
// u_alu_other: its extra port carries a signal (c) that does not enter
// through any original port, so it cannot be folded and the swap is refused.
module top (clk, a, b, bi, ci, c, y, z, keep);
  input clk;
  input [1:0] a;
  input [1:0] b;
  input bi;
  input ci;
  input c;
  output [1:0] y;
  output [1:0] z;
  output keep;

  wire [1:0] aq;
  wire [1:0] bq;
  wire [1:0] sum;
  wire a0_dup;
  wire b1_dup;
  wire cq;
  wire c_buf;
  wire [1:0] sum_other;

  DFF_X1 ra0 (.D(a[0]), .CK(clk), .Q(aq[0]));
  DFF_X1 ra1 (.D(a[1]), .CK(clk), .Q(aq[1]));
  DFF_X1 rb0 (.D(b[0]), .CK(clk), .Q(bq[0]));
  DFF_X1 rb1 (.D(b[1]), .CK(clk), .Q(bq[1]));

  BUF_X1 dupbuf_a0 (.A(aq[0]), .Z(a0_dup));
  BUF_X1 dupbuf_b1 (.A(bq[1]), .Z(b1_dup));
  DFF_X1 rkeep (.D(b1_dup), .CK(clk), .Q(keep));
  DFF_X1 rc (.D(c), .CK(clk), .Q(cq));
  BUF_X1 otherbuf_c (.A(cq), .Z(c_buf));

  ALU_2_0_2_0_2_unused_CO_X_HAN_CARLSON u_alu (
      .A(aq),
      .B(bq),
      .BI(bi),
      .CI(ci),
      .Y(sum),
      .dupA0(a0_dup),
      .dupB1(b1_dup)
  );

  ALU_2_1_2_1_2_unused_CO_X_HAN_CARLSON u_alu_other (
      .A(aq),
      .B(bq),
      .BI(bi),
      .CI(ci),
      .Y(sum_other),
      .other(c_buf)
  );

  DFF_X1 ry0 (.D(sum[0]), .CK(clk), .Q(y[0]));
  DFF_X1 ry1 (.D(sum[1]), .CK(clk), .Q(y[1]));
  DFF_X1 rz0 (.D(sum_other[0]), .CK(clk), .Q(z[0]));
  DFF_X1 rz1 (.D(sum_other[1]), .CK(clk), .Q(z[1]));
endmodule

(* arithmetic_operator = 1 *)
(* implements_operator = "ALU_2_0_2_0_2_unused_CO_X" *)
(* architecture = "HAN_CARLSON" *)
module ALU_2_0_2_0_2_unused_CO_X_HAN_CARLSON (A, B, BI, CI, Y, dupA0, dupB1);
  input [1:0] A;
  input [1:0] B;
  input BI;
  input CI;
  output [1:0] Y;
  input dupA0;
  input dupB1;

  XOR2_X1 x0 (.A(dupA0), .B(B[0]), .Z(Y[0]));
  XOR2_X1 x1 (.A(A[1]), .B(dupB1), .Z(Y[1]));
endmodule

(* arithmetic_operator = 1 *)
(* implements_operator = "ALU_2_0_2_0_2_unused_CO_X" *)
(* architecture = "KOGGE_STONE" *)
module ALU_2_0_2_0_2_unused_CO_X_KOGGE_STONE (A, B, BI, CI, Y);
  input [1:0] A;
  input [1:0] B;
  input BI;
  input CI;
  output [1:0] Y;

  XOR2_X1 x0 (.A(A[0]), .B(B[0]), .Z(Y[0]));
  XOR2_X1 x1 (.A(A[1]), .B(B[1]), .Z(Y[1]));
endmodule

(* arithmetic_operator = 1 *)
(* implements_operator = "ALU_2_1_2_1_2_unused_CO_X" *)
(* architecture = "HAN_CARLSON" *)
module ALU_2_1_2_1_2_unused_CO_X_HAN_CARLSON (A, B, BI, CI, Y, other);
  input [1:0] A;
  input [1:0] B;
  input BI;
  input CI;
  output [1:0] Y;
  input other;

  XOR2_X1 x0 (.A(A[0]), .B(other), .Z(Y[0]));
  XOR2_X1 x1 (.A(A[1]), .B(B[1]), .Z(Y[1]));
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

  XOR2_X1 x0 (.A(A[0]), .B(B[0]), .Z(Y[0]));
  XOR2_X1 x1 (.A(A[1]), .B(B[1]), .Z(Y[1]));
endmodule
