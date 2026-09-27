// TOP: top
// TECH: nangate45
// TARGETS: case_sensitivity, instance, depth_1
// CLUE: Instance names g, G, gG differing only by case, on functionally distinct gates.
module top (a, b, y);
  input a, b;
  output y;
  wire n1, n2;
  NAND2_X1 g (.A1(a), .A2(b), .ZN(n1));
  NOR2_X1 G (.A1(a), .A2(n1), .ZN(n2));
  XOR2_X1 gG (.A(n1), .B(n2), .Z(y));
endmodule
