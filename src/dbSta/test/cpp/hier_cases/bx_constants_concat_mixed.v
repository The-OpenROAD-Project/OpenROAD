// TOP: top
// TECH: nangate45
// TARGETS: concat_const, part_select
// CLUE: constants embedded at both ends of a concat {1'b0, x[2:1], 1'b1}
// driving a bus port — per-bit alignment of const and signal slices.
module sub4 (input [3:0] bus, output y);
  wire n1, n2;
  AND2_X1 g1 (.A1(bus[0]), .A2(bus[1]), .ZN(n1));
  AND2_X1 g2 (.A1(bus[2]), .A2(bus[3]), .ZN(n2));
  OR2_X1 g3 (.A1(n1), .A2(n2), .ZN(y));
endmodule

module top (input [2:0] x, output y);
  wire t;
  sub4 s1 (.bus({1'b0, x[2:1], 1'b1}), .y(t));
  XOR2_X1 gx (.A(t), .B(x[0]), .Z(y));
endmodule
