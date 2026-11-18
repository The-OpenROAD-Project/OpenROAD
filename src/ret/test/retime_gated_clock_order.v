// Two chains, rc on clk and rg on the gated clock.  The groups are rebuilt
// in the order of their nets (a_c, a_g, c3, g4): rc and rg are removed
// before the registers that replace them are created, and the new register
// of the clk chain takes the slot rg had.  The new register of the gated
// chain must still be on gclk.
module top (a, b, en, clk, y, z);
  input a;
  input b;
  input en;
  input clk;
  output y;
  output z;
  wire gclk, a_c, a_g, qc, qg, c1, c2, c3, c4, g1, g2, g3, g4, g5, qc2, qg2;

  CLKGATE_X1 icg (.CK(clk), .E(en), .GCK(gclk));

  BUF_X1 c0 (.A(a), .Z(a_c));
  DFF_X1 rc (.D(a_c), .CK(clk), .Q(qc));
  INV_X1 i1 (.A(qc), .ZN(c1));
  INV_X1 i2 (.A(c1), .ZN(c2));
  INV_X1 i3 (.A(c2), .ZN(c3));
  INV_X1 i4 (.A(c3), .ZN(c4));
  DFF_X1 rc2 (.D(c4), .CK(clk), .Q(qc2));
  BUF_X1 o1 (.A(qc2), .Z(y));

  BUF_X1 gb (.A(b), .Z(a_g));
  DFF_X1 rg (.D(a_g), .CK(gclk), .Q(qg));
  INV_X1 j1 (.A(qg), .ZN(g1));
  INV_X1 j2 (.A(g1), .ZN(g2));
  INV_X1 j3 (.A(g2), .ZN(g3));
  INV_X1 j4 (.A(g3), .ZN(g4));
  INV_X1 j5 (.A(g4), .ZN(g5));
  DFF_X1 rg2 (.D(g5), .CK(gclk), .Q(qg2));
  BUF_X1 o2 (.A(qg2), .Z(z));
endmodule
