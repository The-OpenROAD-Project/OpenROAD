// TOP: top
// TECH: nangate45
// TARGETS: chained_submodule_feedthroughs, depth_1_x4
// CLUE: four pure-assign feedthrough submodule instances in series at top level.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input zi, output o, output zo);
  wire m1, m2, m3;
  ft u0 (.a(i), .y(m1));
  ft u1 (.a(m1), .y(m2));
  ft u2 (.a(m2), .y(m3));
  ft u3 (.a(m3), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
