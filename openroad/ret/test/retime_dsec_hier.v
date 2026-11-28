// Hierarchical netlist.  Inside `core`, r1 and r2 feed one NAND followed by
// a longer cone, all in that module: they merge and slide as in
// retime_slack_search, and the new register takes the module's clock port
// `ck`.  `stage` is combinational and the registers before and after it are
// in `top`, so they stay: a move would take one across a module boundary.
// In `chain` the first register's input comes through the module port from
// `top`, so it stays as well.  `mid` holds `leaf_a` and `leaf_b`: in each
// leaf two inverting registers (DFF_X1 read through QN) feed a NAND, which
// is not self-dual, so they are unmapped to a plain register plus an
// inverter before they merge.  The cells that replace them are created in
// their own leaf, two levels down, and each leaf is handled on its own.
// leaf_b has the shorter cone.  g3 is an INV_X16: its load slows the NAND
// enough that the first slide across it pays for the inverter the
// unmapping adds.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value and the result can be checked with sequential equivalence.
module core (a, b, rn, ck, y);
  input a;
  input b;
  input rn;
  input ck;
  output y;
  wire d1, d2, q1, q2, n1, n2, n3, n4, n5, n6, d3, q3;

  AND2_X1 g0 (.A1(a), .A2(rn), .ZN(d1));
  AND2_X1 g1 (.A1(b), .A2(rn), .ZN(d2));
  DFF_X1 r1 (.D(d1), .CK(ck), .Q(q1));
  DFF_X1 r2 (.D(d2), .CK(ck), .Q(q2));
  NAND2_X1 g2 (.A1(q1), .A2(q2), .ZN(n1));
  INV_X1 g3 (.A(n1), .ZN(n2));
  INV_X1 g4 (.A(n2), .ZN(n3));
  INV_X1 g5 (.A(n3), .ZN(n4));
  INV_X1 g6 (.A(n4), .ZN(n5));
  INV_X1 g7 (.A(n5), .ZN(n6));
  AND2_X1 g8 (.A1(n6), .A2(rn), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(ck), .Q(q3));
  BUF_X1 g9 (.A(q3), .Z(y));
endmodule

module stage (i, o);
  input i;
  output o;
  wire s1, s2, s3;

  INV_X1 h1 (.A(i), .ZN(s1));
  INV_X1 h2 (.A(s1), .ZN(s2));
  INV_X1 h3 (.A(s2), .ZN(s3));
  INV_X1 h4 (.A(s3), .ZN(o));
endmodule

module chain (a, rn, ck, y);
  input a;
  input rn;
  input ck;
  output y;
  wire q1, n1, n2, n3, n4, d2;

  DFF_X1 r1 (.D(a), .CK(ck), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  AND2_X1 g5 (.A1(n4), .A2(rn), .ZN(d2));
  DFF_X1 r2 (.D(d2), .CK(ck), .Q(y));
endmodule

module leaf_a (a, b, rn, ck, y);
  input a;
  input b;
  input rn;
  input ck;
  output y;
  wire d1, d2, q1n, q2n, n1, n2, n3, n4, n5, n6, d3, q3;

  AND2_X1 g0 (.A1(a), .A2(rn), .ZN(d1));
  AND2_X1 g1 (.A1(b), .A2(rn), .ZN(d2));
  DFF_X1 r1 (.D(d1), .CK(ck), .QN(q1n));
  DFF_X1 r2 (.D(d2), .CK(ck), .QN(q2n));
  NAND2_X1 g2 (.A1(q1n), .A2(q2n), .ZN(n1));
  INV_X16 g3 (.A(n1), .ZN(n2));
  INV_X1 g4 (.A(n2), .ZN(n3));
  INV_X1 g5 (.A(n3), .ZN(n4));
  INV_X1 g6 (.A(n4), .ZN(n5));
  INV_X1 g7 (.A(n5), .ZN(n6));
  AND2_X1 g8 (.A1(n6), .A2(rn), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(ck), .Q(q3));
  BUF_X1 g9 (.A(q3), .Z(y));
endmodule

module leaf_b (a, b, rn, ck, y);
  input a;
  input b;
  input rn;
  input ck;
  output y;
  wire d1, d2, q1n, q2n, n1, n2, n3, n4, d3, q3;

  AND2_X1 g0 (.A1(a), .A2(rn), .ZN(d1));
  AND2_X1 g1 (.A1(b), .A2(rn), .ZN(d2));
  DFF_X1 r1 (.D(d1), .CK(ck), .QN(q1n));
  DFF_X1 r2 (.D(d2), .CK(ck), .QN(q2n));
  NAND2_X1 g2 (.A1(q1n), .A2(q2n), .ZN(n1));
  INV_X16 g3 (.A(n1), .ZN(n2));
  INV_X1 g4 (.A(n2), .ZN(n3));
  INV_X1 g5 (.A(n3), .ZN(n4));
  AND2_X1 g8 (.A1(n4), .A2(rn), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(ck), .Q(q3));
  BUF_X1 g9 (.A(q3), .Z(y));
endmodule

module mid (a, b, c, d, rn, ck, y, z);
  input a;
  input b;
  input c;
  input d;
  input rn;
  input ck;
  output y;
  output z;

  leaf_a u_a (.a(a), .b(b), .rn(rn), .ck(ck), .y(y));
  leaf_b u_b (.a(c), .b(d), .rn(rn), .ck(ck), .y(z));
endmodule

module top (a, b, c, d, e, f, g, h, rst_n, clk, y, z, w, u, v);
  input a;
  input b;
  input c;
  input d;
  input e;
  input f;
  input g;
  input h;
  input rst_n;
  input clk;
  output y;
  output z;
  output w;
  output u;
  output v;
  wire dc, qc, so, dz, qz, dw, qw;

  core u_core (.a(a), .b(b), .rn(rst_n), .ck(clk), .y(y));
  AND2_X1 t0 (.A1(c), .A2(rst_n), .ZN(dc));
  DFF_X1 rt1 (.D(dc), .CK(clk), .Q(qc));
  stage u_stage (.i(qc), .o(so));
  AND2_X1 t1 (.A1(so), .A2(rst_n), .ZN(dz));
  DFF_X1 rt2 (.D(dz), .CK(clk), .Q(qz));
  BUF_X1 t2 (.A(qz), .Z(z));
  AND2_X1 t3 (.A1(d), .A2(rst_n), .ZN(dw));
  chain u_chain (.a(dw), .rn(rst_n), .ck(clk), .y(qw));
  BUF_X1 t4 (.A(qw), .Z(w));
  mid u_mid (.a(e), .b(f), .c(g), .d(h), .rn(rst_n), .ck(clk), .y(u), .z(v));
endmodule
