// TOP: top
// TECH: nangate45
// TARGETS: assign_chain_2, submodule_feedthrough, internal_top_wire
// CLUE: SCALAR control for the same bracket: sub feedthrough onto an internal top wire forwarded by a top assign; no buses anywhere.

module sub (input a, output y);
  assign y = a;
endmodule

module top (input i, input zi, output o, output zo);
  wire m;
  sub u0 (.a(i), .y(m));
  assign o = m;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
