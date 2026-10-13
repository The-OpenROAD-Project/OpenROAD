// TOP: top
// TECH: nangate45
// TARGETS: assign_chain_4, top_level
// CLUE: four chained assigns through three intermediate nets at top level.

module top (input i, input zi, output o, output zo);
  wire t1, t2, t3;
  assign t1 = i;
  assign t2 = t1;
  assign t3 = t2;
  assign o = t3;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
