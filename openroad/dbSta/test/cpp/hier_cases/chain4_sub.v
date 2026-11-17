// TOP: top
// TECH: nangate45
// TARGETS: assign_chain_4, submodule_feedthrough
// CLUE: four chained assigns inside a cell-free submodule.

module ft4 (input a, output y);
  wire m1, m2, m3;
  assign m1 = a;
  assign m2 = m1;
  assign m3 = m2;
  assign y = m3;
endmodule

module top (input i, input zi, output o, output zo);
  ft4 u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
