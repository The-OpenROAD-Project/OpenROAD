// TOP: top
// TECH: nangate45
// TARGETS: alias_fanout, two_assign_paths
// CLUE: the SAME input reaches two outputs via assign paths of different length.

module top (input i, input zi, output o1, output o2, output zo);
  wire t;
  assign o1 = i;
  assign t = i;
  assign o2 = t;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
