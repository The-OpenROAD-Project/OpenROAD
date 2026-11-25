// A netlist that an earlier retime run already went through: it holds
// instances and nets with the names retime and the unmapper generate
// (ret_ff_N, ret_net_N, unmap_ff_N, unmap_inv_N, unmap_net_N, unmap_state_N),
// on cells that stay in the design.  New cells and nets must get names that
// are still free.
//   cone 1  r1, r2 -> NAND: plain registers that merge into one new register.
//           ret_ff_0 / ret_net_0 are taken by the output register and
//           ret_net_1 by a net, so the new registers start at ret_ff_2.
//   cone 2  r4, r5 (read through QN) -> NAND: both are unmapped first, which
//           creates unmap_* cells next to the ones already there.  g13 is
//           an INV_X16: its load slows g12 enough that the first slide
//           across g12 pays for the inverter the unmapping adds.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, b, c, e, rst_n, clk, y, z);
  input a;
  input b;
  input c;
  input e;
  input rst_n;
  input clk;
  output y;
  output z;
  wire d1, d2, q1, q2, n1, n2, n3, n4, ret_net_1, d3, ret_net_0;
  wire d4, d5, q4n, q5n, m1, m2, m3, m4, unmap_net_3, unmap_state_1, q6;

  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  AND2_X1 g1 (.A1(b), .A2(rst_n), .ZN(d2));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  DFF_X1 r2 (.D(d2), .CK(clk), .Q(q2));
  NAND2_X1 g2 (.A1(q1), .A2(q2), .ZN(n1));
  INV_X1 g3 (.A(n1), .ZN(n2));
  INV_X1 g4 (.A(n2), .ZN(n3));
  INV_X1 g5 (.A(n3), .ZN(n4));
  INV_X1 g6 (.A(n4), .ZN(ret_net_1));
  AND2_X1 g7 (.A1(ret_net_1), .A2(rst_n), .ZN(d3));
  DFF_X1 ret_ff_0 (.D(d3), .CK(clk), .Q(ret_net_0));
  BUF_X1 g8 (.A(ret_net_0), .Z(y));

  AND2_X1 g10 (.A1(c), .A2(rst_n), .ZN(d4));
  AND2_X1 g11 (.A1(e), .A2(rst_n), .ZN(d5));
  DFF_X1 r4 (.D(d4), .CK(clk), .QN(q4n));
  DFF_X1 r5 (.D(d5), .CK(clk), .QN(q5n));
  NAND2_X1 g12 (.A1(q4n), .A2(q5n), .ZN(m1));
  INV_X16 g13 (.A(m1), .ZN(m2));
  INV_X1 g14 (.A(m2), .ZN(m3));
  INV_X1 g15 (.A(m3), .ZN(m4));
  INV_X1 g16 (.A(m4), .ZN(unmap_net_3));
  AND2_X1 g17 (.A1(unmap_net_3), .A2(rst_n), .ZN(unmap_state_1));
  DFF_X1 unmap_ff_0 (.D(unmap_state_1), .CK(clk), .Q(q6));
  BUF_X1 unmap_inv_2 (.A(q6), .Z(z));
endmodule
