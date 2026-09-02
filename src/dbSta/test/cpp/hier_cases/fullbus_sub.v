// TOP: top
// TECH: nangate45
// TARGETS: full_bus_assign, submodule_feedthrough
// CLUE: whole-bus feedthrough assign (no part select) inside a cell-free submodule.

module sub (input [3:0] a, output [3:0] y);
  assign y = a;
endmodule

module top (input [3:0] i, input zi, output [3:0] o, output zo);
  sub u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
