// Three cones with the same 14-inverter path from a register to a plain
// dfxtp (sky130hd).
//   cone 1  r1 is a scan flop (sdfxtp, a liberty test_cell): it stays.
//   cone 2  r3 has an asynchronous set driven by the port set_n (dfstp): it
//           stays.
//   cone 3  r5 is a plain dfxtp: it slides forward.
module top (a, b, c, d, e, f, scan_in, scan_en, set_n, clk, x, y, z);
  input a;
  input b;
  input c;
  input d;
  input e;
  input f;
  input scan_in;
  input scan_en;
  input set_n;
  input clk;
  output x;
  output y;
  output z;
  wire sd, sq, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, sr;
  wire pd, pq, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, pr;
  wire md, mq, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, mr;

  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(b), .X(sd));
  sky130_fd_sc_hd__sdfxtp_1 r1 (.D(sd), .SCD(scan_in), .SCE(scan_en), .CLK(clk), .Q(sq));
  sky130_fd_sc_hd__inv_1 g1 (.A(sq), .Y(s1));
  sky130_fd_sc_hd__inv_1 g2 (.A(s1), .Y(s2));
  sky130_fd_sc_hd__inv_1 g3 (.A(s2), .Y(s3));
  sky130_fd_sc_hd__inv_1 g4 (.A(s3), .Y(s4));
  sky130_fd_sc_hd__inv_1 g5 (.A(s4), .Y(s5));
  sky130_fd_sc_hd__inv_1 g6 (.A(s5), .Y(s6));
  sky130_fd_sc_hd__inv_1 g7 (.A(s6), .Y(s7));
  sky130_fd_sc_hd__inv_1 g8 (.A(s7), .Y(s8));
  sky130_fd_sc_hd__inv_1 g9 (.A(s8), .Y(s9));
  sky130_fd_sc_hd__inv_1 g10 (.A(s9), .Y(s10));
  sky130_fd_sc_hd__inv_1 g11 (.A(s10), .Y(s11));
  sky130_fd_sc_hd__inv_1 g12 (.A(s11), .Y(s12));
  sky130_fd_sc_hd__inv_1 g13 (.A(s12), .Y(s13));
  sky130_fd_sc_hd__inv_1 g14 (.A(s13), .Y(s14));
  sky130_fd_sc_hd__dfxtp_1 r2 (.D(s14), .CLK(clk), .Q(sr));
  sky130_fd_sc_hd__buf_1 g15 (.A(sr), .X(x));

  sky130_fd_sc_hd__and2_1 g20 (.A(c), .B(d), .X(pd));
  sky130_fd_sc_hd__dfstp_1 r3 (.D(pd), .SET_B(set_n), .CLK(clk), .Q(pq));
  sky130_fd_sc_hd__inv_1 g21 (.A(pq), .Y(p1));
  sky130_fd_sc_hd__inv_1 g22 (.A(p1), .Y(p2));
  sky130_fd_sc_hd__inv_1 g23 (.A(p2), .Y(p3));
  sky130_fd_sc_hd__inv_1 g24 (.A(p3), .Y(p4));
  sky130_fd_sc_hd__inv_1 g25 (.A(p4), .Y(p5));
  sky130_fd_sc_hd__inv_1 g26 (.A(p5), .Y(p6));
  sky130_fd_sc_hd__inv_1 g27 (.A(p6), .Y(p7));
  sky130_fd_sc_hd__inv_1 g28 (.A(p7), .Y(p8));
  sky130_fd_sc_hd__inv_1 g29 (.A(p8), .Y(p9));
  sky130_fd_sc_hd__inv_1 g30 (.A(p9), .Y(p10));
  sky130_fd_sc_hd__inv_1 g31 (.A(p10), .Y(p11));
  sky130_fd_sc_hd__inv_1 g32 (.A(p11), .Y(p12));
  sky130_fd_sc_hd__inv_1 g33 (.A(p12), .Y(p13));
  sky130_fd_sc_hd__inv_1 g34 (.A(p13), .Y(p14));
  sky130_fd_sc_hd__dfxtp_1 r4 (.D(p14), .CLK(clk), .Q(pr));
  sky130_fd_sc_hd__buf_1 g35 (.A(pr), .X(y));

  sky130_fd_sc_hd__and2_1 g40 (.A(e), .B(f), .X(md));
  sky130_fd_sc_hd__dfxtp_1 r5 (.D(md), .CLK(clk), .Q(mq));
  sky130_fd_sc_hd__inv_1 g41 (.A(mq), .Y(m1));
  sky130_fd_sc_hd__inv_1 g42 (.A(m1), .Y(m2));
  sky130_fd_sc_hd__inv_1 g43 (.A(m2), .Y(m3));
  sky130_fd_sc_hd__inv_1 g44 (.A(m3), .Y(m4));
  sky130_fd_sc_hd__inv_1 g45 (.A(m4), .Y(m5));
  sky130_fd_sc_hd__inv_1 g46 (.A(m5), .Y(m6));
  sky130_fd_sc_hd__inv_1 g47 (.A(m6), .Y(m7));
  sky130_fd_sc_hd__inv_1 g48 (.A(m7), .Y(m8));
  sky130_fd_sc_hd__inv_1 g49 (.A(m8), .Y(m9));
  sky130_fd_sc_hd__inv_1 g50 (.A(m9), .Y(m10));
  sky130_fd_sc_hd__inv_1 g51 (.A(m10), .Y(m11));
  sky130_fd_sc_hd__inv_1 g52 (.A(m11), .Y(m12));
  sky130_fd_sc_hd__inv_1 g53 (.A(m12), .Y(m13));
  sky130_fd_sc_hd__inv_1 g54 (.A(m13), .Y(m14));
  sky130_fd_sc_hd__dfxtp_1 r6 (.D(m14), .CLK(clk), .Q(mr));
  sky130_fd_sc_hd__buf_1 g55 (.A(mr), .X(z));
endmodule
