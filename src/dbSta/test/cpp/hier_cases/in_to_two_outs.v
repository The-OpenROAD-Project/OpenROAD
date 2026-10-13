// TOP: top
// TECH: nangate45
// TARGETS: alias_input_to_two_outputs
// CLUE: one input aliased directly onto two different output ports.

module top (input i, input zi, output o1, output o2, output zo);
  assign o1 = i;
  assign o2 = i;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
