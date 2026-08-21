// TOP: top
// TECH: nangate45
// TARGETS: feedthrough_plus_internal_load, top_level
// CLUE: input drives both an assign-to-output alias AND a gate pin at top level.

module top (input i, output o1, output o2);
  assign o1 = i;
  INV_X1 g0 (.A(i), .ZN(o2));
endmodule
