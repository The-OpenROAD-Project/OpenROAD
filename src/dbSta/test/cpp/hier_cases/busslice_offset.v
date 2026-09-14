// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, nonzero_lsb, submodule_feedthrough
// CLUE: finding-2 shape with non-zero-based bus declarations [7:4]/[5:4].

module sub (input [7:4] a, output [5:4] y);
  assign y = a[7:6];
endmodule

module top (input [7:4] i, input zi, output [5:4] o, output zo);
  sub u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
