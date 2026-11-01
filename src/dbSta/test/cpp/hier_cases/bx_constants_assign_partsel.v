// TOP: top
// TECH: nangate45
// TARGETS: assign_const_wire, part_select
// CLUE: a 4-bit wire assembled from constant part-select assigns
// (w[2:1]=2'b10, w[0]=1'b1, w[3]=1'b0) then consumed by gates — per-bit
// constant bookkeeping across part selects.
module top (input a, output y);
  wire [3:0] w;
  wire n1, n2, n3;
  assign w[2:1] = 2'b10;
  assign w[0] = 1'b1;
  assign w[3] = 1'b0;
  AND2_X1 g1 (.A1(w[0]), .A2(w[1]), .ZN(n1));
  AND2_X1 g2 (.A1(w[2]), .A2(w[3]), .ZN(n2));
  OR2_X1 g3 (.A1(n1), .A2(n2), .ZN(n3));
  XOR2_X1 g4 (.A(n3), .B(a), .Z(y));
endmodule
