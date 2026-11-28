// Four independent cones around the enable pins of sky130hd edfxtp registers.
// The NAND g2 feeds both its inverter chain and the DE pin of rX: moving rE
// and rF across g2 would hand the enable the value one cycle early, so g2 is
// pinned and its cone stays.
// rG's Q feeds both the NAND h2 and, directly, the DE pin of rY: moving rG
// across h2 would hand the enable the value one cycle early, so rG (and rH,
// which merges with it) stay.
// rc drives only the DE pin of r2, a control pin, so no edge of the retiming
// graph leaves the delay buffer i4.  Crossing i4 would take a register from
// in front of it and put none behind it: r2 would see its enable one cycle
// early.  i4 is never crossed, and r1 slides up to it.
// rA (plain) feeds rB (enable en); rB and rC (enable en) feed the NAND c2 and
// merge into a single edfxtp after it.  rA stays in front of c2 and must be
// rebuilt as a plain register, not as an enable register.  rD (plain) shares
// rA's data net, so the two are rebuilt as one register.
// rst_n is a synchronous reset in logic, and en is forced on while it is
// asserted.  Meanwhile rE, rF and rG are 0, so rX loads 0 and rY keeps the
// value it starts from, and every register settles to a defined value for
// the sequential equivalence check.
module top (a, b, c, d, e, f, g, h, i, j, en_i, rst_n, clk, y, z, u, v, w, s,
            t);
  input a;
  input b;
  input c;
  input d;
  input e;
  input f;
  input g;
  input h;
  input i;
  input j;
  input en_i;
  input rst_n;
  input clk;
  output y;
  output z;
  output u;
  output v;
  output w;
  output s;
  output t;
  wire d1, d2, dx, qe, qf, qx, n1, n2, n3, n4, n5, n6, d3, q3;
  wire e1, e2, ey, qg, qh, qy, k1, k2, k3, k4, k5, k6, e3, q13;
  wire f1, q1, s1, s2, s3, s4, qc, f2, q2;
  wire rst, en, b1, b2, qa, qb, qr, qd, m1, m2, m3, m4, m5, m6, b3, q23;

  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(rst_n), .X(d1));
  sky130_fd_sc_hd__and2_1 g1 (.A(b), .B(rst_n), .X(d2));
  sky130_fd_sc_hd__and2_1 g1x (.A(c), .B(rst_n), .X(dx));
  sky130_fd_sc_hd__dfxtp_1 rE (.D(d1), .CLK(clk), .Q(qe));
  sky130_fd_sc_hd__dfxtp_1 rF (.D(d2), .CLK(clk), .Q(qf));
  sky130_fd_sc_hd__edfxtp_1 rX (.D(dx), .DE(n1), .CLK(clk), .Q(qx));
  sky130_fd_sc_hd__buf_1 g_z (.A(qx), .X(z));
  sky130_fd_sc_hd__nand2_1 g2 (.A(qe), .B(qf), .Y(n1));
  sky130_fd_sc_hd__inv_1 g3 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__inv_1 g4 (.A(n2), .Y(n3));
  sky130_fd_sc_hd__inv_1 g5 (.A(n3), .Y(n4));
  sky130_fd_sc_hd__inv_1 g6 (.A(n4), .Y(n5));
  sky130_fd_sc_hd__inv_1 g7 (.A(n5), .Y(n6));
  sky130_fd_sc_hd__and2_1 g8 (.A(n6), .B(rst_n), .X(d3));
  sky130_fd_sc_hd__dfxtp_1 r3 (.D(d3), .CLK(clk), .Q(q3));
  sky130_fd_sc_hd__buf_1 g9 (.A(q3), .X(y));

  sky130_fd_sc_hd__and2_1 h0 (.A(d), .B(rst_n), .X(e1));
  sky130_fd_sc_hd__and2_1 h1 (.A(e), .B(rst_n), .X(e2));
  sky130_fd_sc_hd__and2_1 h1x (.A(f), .B(rst_n), .X(ey));
  sky130_fd_sc_hd__dfxtp_1 rG (.D(e1), .CLK(clk), .Q(qg));
  sky130_fd_sc_hd__dfxtp_1 rH (.D(e2), .CLK(clk), .Q(qh));
  sky130_fd_sc_hd__edfxtp_1 rY (.D(ey), .DE(qg), .CLK(clk), .Q(qy));
  sky130_fd_sc_hd__buf_1 h_v (.A(qy), .X(v));
  sky130_fd_sc_hd__nand2_1 h2 (.A(qg), .B(qh), .Y(k1));
  sky130_fd_sc_hd__inv_1 h3 (.A(k1), .Y(k2));
  sky130_fd_sc_hd__inv_1 h4 (.A(k2), .Y(k3));
  sky130_fd_sc_hd__inv_1 h5 (.A(k3), .Y(k4));
  sky130_fd_sc_hd__inv_1 h6 (.A(k4), .Y(k5));
  sky130_fd_sc_hd__inv_1 h7 (.A(k5), .Y(k6));
  sky130_fd_sc_hd__and2_1 h8 (.A(k6), .B(rst_n), .X(e3));
  sky130_fd_sc_hd__dfxtp_1 r13 (.D(e3), .CLK(clk), .Q(q13));
  sky130_fd_sc_hd__buf_1 h9 (.A(q13), .X(u));

  sky130_fd_sc_hd__and2_1 j0 (.A(g), .B(rst_n), .X(f1));
  sky130_fd_sc_hd__dfxtp_1 r1 (.D(f1), .CLK(clk), .Q(q1));
  sky130_fd_sc_hd__inv_1 i1 (.A(q1), .Y(s1));
  sky130_fd_sc_hd__inv_1 i2 (.A(s1), .Y(s2));
  sky130_fd_sc_hd__inv_1 i3 (.A(s2), .Y(s3));
  sky130_fd_sc_hd__clkdlybuf4s50_1 i4 (.A(s3), .X(s4));
  sky130_fd_sc_hd__dfxtp_1 rc (.D(s4), .CLK(clk), .Q(qc));
  sky130_fd_sc_hd__and2_1 j1 (.A(h), .B(rst_n), .X(f2));
  sky130_fd_sc_hd__edfxtp_1 r2 (.D(f2), .DE(qc), .CLK(clk), .Q(q2));
  sky130_fd_sc_hd__buf_1 j9 (.A(q2), .X(w));

  sky130_fd_sc_hd__inv_1 g_r (.A(rst_n), .Y(rst));
  sky130_fd_sc_hd__or2_1 g_e1 (.A(en_i), .B(rst), .X(en));
  sky130_fd_sc_hd__and2_1 c0 (.A(i), .B(rst_n), .X(b1));
  sky130_fd_sc_hd__and2_1 c1 (.A(j), .B(rst_n), .X(b2));
  sky130_fd_sc_hd__dfxtp_1 rA (.D(b1), .CLK(clk), .Q(qa));
  sky130_fd_sc_hd__edfxtp_1 rB (.D(qa), .DE(en), .CLK(clk), .Q(qb));
  sky130_fd_sc_hd__edfxtp_1 rC (.D(b2), .DE(en), .CLK(clk), .Q(qr));
  sky130_fd_sc_hd__dfxtp_1 rD (.D(b1), .CLK(clk), .Q(qd));
  sky130_fd_sc_hd__buf_1 c_t (.A(qd), .X(t));
  sky130_fd_sc_hd__nand2_1 c2 (.A(qb), .B(qr), .Y(m1));
  sky130_fd_sc_hd__inv_1 c3 (.A(m1), .Y(m2));
  sky130_fd_sc_hd__inv_1 c4 (.A(m2), .Y(m3));
  sky130_fd_sc_hd__inv_1 c5 (.A(m3), .Y(m4));
  sky130_fd_sc_hd__inv_1 c6 (.A(m4), .Y(m5));
  sky130_fd_sc_hd__inv_1 c7 (.A(m5), .Y(m6));
  sky130_fd_sc_hd__and2_1 c8 (.A(m6), .B(rst_n), .X(b3));
  sky130_fd_sc_hd__dfxtp_1 r23 (.D(b3), .CLK(clk), .Q(q23));
  sky130_fd_sc_hd__buf_1 c9 (.A(q23), .X(s));
endmodule
