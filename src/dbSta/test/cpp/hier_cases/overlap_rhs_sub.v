// TOP: top
// TECH: nangate45
// TARGETS: overlapping_rhs_slices, submodule_feedthrough
// CLUE: two slice assigns whose RHS ranges overlap (a[2] feeds both y[1] and y[2]).

module sub (input [3:0] a, output [3:0] y);
  assign y[1:0] = a[2:1];
  assign y[3:2] = a[3:2];
endmodule

module top (input [3:0] i, input zi, output [3:0] o, output zo);
  sub u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
