// TOP: top
// TECH: nangate45
// TARGETS: concat_duplicated_bits
// CLUE: input port concat duplicates bits {i[1],i[1],i[0],i[0]}; the same top net appears twice inside one port connection expression.
module leaf (input [3:0] a, output [3:0] z);
  INV_X1   g0 (.A(a[0]), .ZN(z[0]));
  BUF_X1   g1 (.A(a[1]), .Z(z[1]));
  NAND2_X1 g2 (.A1(a[2]), .A2(a[0]), .ZN(z[2]));
  NOR2_X1  g3 (.A1(a[3]), .A2(a[1]), .ZN(z[3]));
endmodule
module top (input [1:0] i, output [3:0] o);
  leaf u0 (.a({i[1], i[1], i[0], i[0]}), .z(o));
endmodule
