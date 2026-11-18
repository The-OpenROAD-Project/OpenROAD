// TOP: top
// TECH: nangate45
// TARGETS: cross_slice_swap, submodule_feedthrough
// CLUE: two bus-slice assigns that swap halves of the bus inside a submodule.

module sub (input [3:0] a, output [3:0] y);
  assign y[3:2] = a[1:0];
  assign y[1:0] = a[3:2];
endmodule

module top (input [3:0] i, input zi, output [3:0] o, output zo);
  sub u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
