// TOP: top
// TECH: nangate45
// TARGETS: bus_const, wide_literal
// CLUE: 8-bit bus port tied to 8'hA5 — wider-than-nibble constant, per-bit
// expansion across two hex digits.
module sub8 (input [7:0] bus, output y);
  wire n1, n2, n3, n4, m1, m2;
  AND2_X1 g1 (.A1(bus[0]), .A2(bus[1]), .ZN(n1));
  AND2_X1 g2 (.A1(bus[2]), .A2(bus[3]), .ZN(n2));
  AND2_X1 g3 (.A1(bus[4]), .A2(bus[5]), .ZN(n3));
  AND2_X1 g4 (.A1(bus[6]), .A2(bus[7]), .ZN(n4));
  OR2_X1 g5 (.A1(n1), .A2(n2), .ZN(m1));
  OR2_X1 g6 (.A1(n3), .A2(n4), .ZN(m2));
  OR2_X1 g7 (.A1(m1), .A2(m2), .ZN(y));
endmodule

module top (input a, output y);
  wire t;
  sub8 s1 (.bus(8'hA5), .y(t));
  XOR2_X1 gx (.A(t), .B(a), .Z(y));
endmodule
