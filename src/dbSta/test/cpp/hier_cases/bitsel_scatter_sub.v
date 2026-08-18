// TOP: top
// TECH: nangate45
// TARGETS: bit_select_assign, submodule_feedthrough
// CLUE: four independent single-bit feedthrough assigns permuting bits inside a submodule.

module sub (input [3:0] a, output [3:0] y);
  assign y[0] = a[3];
  assign y[1] = a[2];
  assign y[2] = a[0];
  assign y[3] = a[1];
endmodule

module top (input [3:0] i, input zi, output [3:0] o, output zo);
  sub u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
