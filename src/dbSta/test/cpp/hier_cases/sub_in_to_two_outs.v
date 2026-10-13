// TOP: top
// TECH: nangate45
// TARGETS: alias_input_to_two_outputs, submodule
// CLUE: one submodule input feeds two submodule outputs through two separate assigns (same input reaching two outputs inside the child).

module s (input a, output y1, output y2);
  assign y1 = a;
  assign y2 = a;
endmodule

module top (input i, input zi, output o1, output o2, output zo);
  s u0 (.a(i), .y1(o1), .y2(o2));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
