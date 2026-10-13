// TOP: top
// TECH: nangate45
// TARGETS: tie0, tie1, all_const_gate
// CLUE: a gate whose inputs are BOTH constant (1'b0 and 1'b1); its output is a
// constant net feeding live logic — const-propagation shortcuts may misfire.
module top (input a, output y);
  wire t;
  AND2_X1 g1 (.A1(1'b0), .A2(1'b1), .ZN(t));
  OR2_X1 g2 (.A1(a), .A2(t), .ZN(y));
endmodule
