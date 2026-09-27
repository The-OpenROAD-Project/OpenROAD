// TOP: top
// TECH: nangate45
// TARGETS: escaped_name_net, assign_chain_2
// CLUE: escaped-name net (punctuation) in the middle of a top-level assign chain.

module top (input i, input zi, output o, output zo);
  wire \net!alias ;
  assign \net!alias  = i;
  assign o = \net!alias ;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
