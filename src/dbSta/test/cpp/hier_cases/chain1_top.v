// TOP: top
// TECH: nangate45
// TARGETS: assign_chain_1, top_level
// CLUE: single wire-to-wire feedthrough assign top input -> top output; baseline.

module top (input i, input zi, output o, output zo);
  assign o = i;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
