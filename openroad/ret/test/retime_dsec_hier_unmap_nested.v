// Two levels of hierarchy: `top` holds `mid`, `mid` holds `leaf_a` and
// `leaf_b`.  In each leaf two inverting registers (DFF_X1 read through QN)
// feed a NAND, which is not self-dual, so they are unmapped to a plain
// register plus an inverter before they merge.  The cells that replace them
// are created in their own leaf, two levels down, and each leaf is handled
// on its own.  leaf_b has the shorter cone.  g3 is an INV_X16: its load
// slows the NAND enough that the first slide across it pays for the inverter
// the unmapping adds.  rst_n is a synchronous reset in logic, so every
// register settles to a defined value for the sequential equivalence check.
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

module top (a, b, c, d, rst_n, clk, y, z);
  input a;
  input b;
  input c;
  input d;
  input rst_n;
  input clk;
  output y;
  output z;

  mid u_mid (.a(a), .b(b), .c(c), .d(d), .rn(rst_n), .ck(clk), .y(y), .z(z));
endmodule
