// TOP: top
// TECH: nangate45
// TARGETS: alias_fanout, two_submodules
// CLUE: one input feeds two separate feedthrough submodule instances to two outputs.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input zi, output o1, output o2, output zo);
  ft u0 (.a(i), .y(o1));
  ft u1 (.a(i), .y(o2));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
