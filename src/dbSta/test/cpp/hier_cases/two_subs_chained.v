// TOP: top
// TECH: nangate45
// TARGETS: chained_submodule_feedthroughs
// CLUE: two feedthrough submodules in series joined by a top-level wire.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input zi, output o, output zo);
  wire m;
  ft u0 (.a(i), .y(m));
  ft u1 (.a(m), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
