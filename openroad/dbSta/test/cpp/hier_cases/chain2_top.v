// TOP: top
// TECH: nangate45
// TARGETS: assign_chain_2, top_level
// CLUE: two chained assigns through one intermediate net; writer may collapse or drop.

module top (input i, input zi, output o, output zo);
  wire t1;
  assign t1 = i;
  assign o = t1;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
