// TOP: top
// TECH: nangate45
// TARGETS: assign_chain_2, submodule_feedthrough
// CLUE: two chained assigns inside a cell-free submodule.

module ft2 (input a, output y);
  wire m;
  assign m = a;
  assign y = m;
endmodule

module top (input i, input zi, output o, output zo);
  ft2 u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
