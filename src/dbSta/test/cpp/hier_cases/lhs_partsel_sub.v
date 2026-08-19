// TOP: top
// TECH: nangate45
// TARGETS: lhs_part_select, submodule_feedthrough
// CLUE: part-select and bit-select on the LHS of feedthrough assigns inside a submodule.

module sub (input [3:0] a, output [3:0] y);
  assign y[2:1] = a[1:0];
  assign y[0] = a[3];
  assign y[3] = a[2];
endmodule

module top (input [3:0] i, input zi, output [3:0] o, output zo);
  sub u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
