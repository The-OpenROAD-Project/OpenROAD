// Four independent cones.
// rp is fed straight from the input port a, so no gate drives it and it is
// on no edge of the retiming graph.  g2 reads rp and r1: moving r1 across g2
// would leave rp behind and a would reach g2 one cycle late, so g2 is never
// crossed.
// t is tapped from the middle of a register chain: it reads rA one cycle
// after c, so the chain rA -> rB -> rD in front of g22 must stay as it is.
// Moving rD across g22 would rebuild the chain two deep and t would read it
// at the wrong depth.
// rz drives only the output port z, so no edge of the retiming graph leaves
// the delay buffer i4.  Crossing i4 would take a register from in front of it
// and put none behind it: z would read e one cycle early.  i4 is never
// crossed, and r8 slides up to it.
// The last cone (r4, r5 -> NAND) has a gate in front of each register and no
// port on its chain, and it retimes.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, b, c, d, e, f, g, rst_n, clk, y, t, x, z, w);
  input a;
  input b;
  input c;
  input d;
  input e;
  input f;
  input g;
  input rst_n;
  input clk;
  output y;
  output t;
  output x;
  output z;
  output w;
  wire qp, d1, q1, n1, n2, n3, n4, n5, n6, d3, q3;
  wire da, dc, qa, qb, qd, qc, p1, p2, p3, p4, p5, p6, d7, q7;
  wire d8, q8, s1, s2, s3, s4;
  wire d4, d5, q4, q5, m1, m2, m3, m4, m5, d6, q6;

  sky130_fd_sc_hd__dfxtp_1 rp (.D(a), .CLK(clk), .Q(qp));
  sky130_fd_sc_hd__and2_1 g0 (.A(b), .B(rst_n), .X(d1));
  sky130_fd_sc_hd__dfxtp_1 r1 (.D(d1), .CLK(clk), .Q(q1));
  sky130_fd_sc_hd__nand2_1 g2 (.A(qp), .B(q1), .Y(n1));
  sky130_fd_sc_hd__inv_1 g3 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__inv_1 g4 (.A(n2), .Y(n3));
  sky130_fd_sc_hd__inv_1 g5 (.A(n3), .Y(n4));
  sky130_fd_sc_hd__inv_1 g6 (.A(n4), .Y(n5));
  sky130_fd_sc_hd__inv_1 g6b (.A(n5), .Y(n6));
  sky130_fd_sc_hd__and2_1 g7 (.A(n6), .B(rst_n), .X(d3));
  sky130_fd_sc_hd__dfxtp_1 r3 (.D(d3), .CLK(clk), .Q(q3));
  sky130_fd_sc_hd__buf_1 g8 (.A(q3), .X(y));

  sky130_fd_sc_hd__and2_1 g20 (.A(c), .B(rst_n), .X(da));
  sky130_fd_sc_hd__and2_1 g21 (.A(d), .B(rst_n), .X(dc));
  sky130_fd_sc_hd__dfxtp_1 rA (.D(da), .CLK(clk), .Q(qa));
  sky130_fd_sc_hd__dfxtp_1 rB (.D(qa), .CLK(clk), .Q(qb));
  sky130_fd_sc_hd__dfxtp_1 rD (.D(qb), .CLK(clk), .Q(qd));
  sky130_fd_sc_hd__dfxtp_1 rC (.D(dc), .CLK(clk), .Q(qc));
  assign t = qa;
  sky130_fd_sc_hd__nand2_1 g22 (.A(qd), .B(qc), .Y(p1));
  sky130_fd_sc_hd__inv_1 g23 (.A(p1), .Y(p2));
  sky130_fd_sc_hd__inv_1 g24 (.A(p2), .Y(p3));
  sky130_fd_sc_hd__inv_1 g25 (.A(p3), .Y(p4));
  sky130_fd_sc_hd__inv_1 g26 (.A(p4), .Y(p5));
  sky130_fd_sc_hd__inv_1 g27 (.A(p5), .Y(p6));
  sky130_fd_sc_hd__and2_1 g28 (.A(p6), .B(rst_n), .X(d7));
  sky130_fd_sc_hd__dfxtp_1 r7 (.D(d7), .CLK(clk), .Q(q7));
  sky130_fd_sc_hd__buf_1 g29 (.A(q7), .X(x));

  sky130_fd_sc_hd__and2_1 g30 (.A(e), .B(rst_n), .X(d8));
  sky130_fd_sc_hd__dfxtp_1 r8 (.D(d8), .CLK(clk), .Q(q8));
  sky130_fd_sc_hd__inv_1 i1 (.A(q8), .Y(s1));
  sky130_fd_sc_hd__inv_1 i2 (.A(s1), .Y(s2));
  sky130_fd_sc_hd__inv_1 i3 (.A(s2), .Y(s3));
  sky130_fd_sc_hd__clkdlybuf4s50_1 i4 (.A(s3), .X(s4));
  sky130_fd_sc_hd__dfxtp_1 rz (.D(s4), .CLK(clk), .Q(z));

  sky130_fd_sc_hd__and2_1 g10 (.A(f), .B(rst_n), .X(d4));
  sky130_fd_sc_hd__and2_1 g11 (.A(g), .B(rst_n), .X(d5));
  sky130_fd_sc_hd__dfxtp_1 r4 (.D(d4), .CLK(clk), .Q(q4));
  sky130_fd_sc_hd__dfxtp_1 r5 (.D(d5), .CLK(clk), .Q(q5));
  sky130_fd_sc_hd__nand2_1 g12 (.A(q4), .B(q5), .Y(m1));
  sky130_fd_sc_hd__inv_1 g13 (.A(m1), .Y(m2));
  sky130_fd_sc_hd__inv_1 g14 (.A(m2), .Y(m3));
  sky130_fd_sc_hd__inv_1 g15 (.A(m3), .Y(m4));
  sky130_fd_sc_hd__inv_1 g16 (.A(m4), .Y(m5));
  sky130_fd_sc_hd__and2_1 g17 (.A(m5), .B(rst_n), .X(d6));
  sky130_fd_sc_hd__dfxtp_1 r6 (.D(d6), .CLK(clk), .Q(q6));
  sky130_fd_sc_hd__buf_1 g18 (.A(q6), .X(w));
endmodule
