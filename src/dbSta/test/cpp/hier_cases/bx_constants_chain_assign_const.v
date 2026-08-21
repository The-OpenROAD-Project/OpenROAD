// TOP: top
// TECH: nangate45
// TARGETS: assign_const_wire, assign_chain
// CLUE: constant flows through a CHAIN of wire assigns (w1 = 1'b1; w2 = w1;
// w3 = w2) before a gate — alias-chain collapsing over a constant source.
module top (input a, output y);
  wire w1, w2, w3;
  assign w1 = 1'b1;
  assign w2 = w1;
  assign w3 = w2;
  AND2_X1 g (.A1(a), .A2(w3), .ZN(y));
endmodule
