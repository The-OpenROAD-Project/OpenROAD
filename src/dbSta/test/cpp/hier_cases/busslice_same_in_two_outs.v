// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, one_source_two_outputs, submodule
// CLUE: one input slice feeds TWO different output slices of the same submodule through two assigns (bus flavour of the shared-source feedthrough).

module sub (input [3:0] a, output [3:0] y);
  assign y[1:0] = a[3:2];
  assign y[3:2] = a[3:2];
endmodule

module top (input [3:0] i, input zi, output [3:0] o, output zo);
  sub u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
