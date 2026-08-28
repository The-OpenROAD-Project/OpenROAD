// TOP: top
// TECH: nangate45
// TARGETS: concat_mix_const_and_bits
// CLUE: port concat mixing constant literals with net bits {1'b0, i[2:1], 1'b1}; constants inside a port concat need LOGIC0/LOGIC1 or assign material on rewrite.
module leaf (input [3:0] a, output [3:0] z);
  INV_X1   g0 (.A(a[0]), .ZN(z[0]));
  BUF_X1   g1 (.A(a[1]), .Z(z[1]));
  NAND2_X1 g2 (.A1(a[2]), .A2(a[0]), .ZN(z[2]));
  NOR2_X1  g3 (.A1(a[3]), .A2(a[1]), .ZN(z[3]));
endmodule
module top (input [3:0] i, output [3:0] o);
  leaf u0 (.a({1'b0, i[2:1], 1'b1}), .z(o));
endmodule
