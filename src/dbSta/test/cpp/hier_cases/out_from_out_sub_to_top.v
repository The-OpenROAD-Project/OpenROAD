// TOP: top
// TECH: nangate45
// TARGETS: alias_output_from_output, submodule_source
// CLUE: top output o1 is driven by a submodule feedthrough and top output o2 is assigned FROM o1, so the alias root is a dissolved boundary net.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input zi, output o1, output o2, output zo);
  ft u0 (.a(i), .y(o1));
  assign o2 = o1;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
