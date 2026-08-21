// TOP: top
// TECH: nangate45
// TARGETS: alias_fanout, sub_path_and_top_path
// CLUE: the same input reaches o1 through a feedthrough submodule and o2 through a plain top-level assign: two structurally different alias paths, one source.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input zi, output o1, output o2, output zo);
  ft u0 (.a(i), .y(o1));
  assign o2 = i;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
