// TOP: top
// TECH: nangate45
// TARGETS: alias_output_from_output, chain_of_outputs
// CLUE: three output ports chained by assigns: o3 = o2 = o1, only o1 gate-driven.

module top (input i, output o1, output o2, output o3);
  INV_X1 g0 (.A(i), .ZN(o1));
  assign o2 = o1;
  assign o3 = o2;
endmodule
