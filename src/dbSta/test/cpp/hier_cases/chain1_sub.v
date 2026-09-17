// TOP: top
// TECH: nangate45
// TARGETS: assign_chain_1, submodule_feedthrough
// CLUE: submodule containing ONLY a scalar feedthrough assign; flat write must keep the alias.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input zi, output o, output zo);
  ft u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
