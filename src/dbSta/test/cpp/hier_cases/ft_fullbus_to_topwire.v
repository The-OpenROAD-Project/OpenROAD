// TOP: top
// TECH: nangate45
// TARGETS: full_bus_assign, submodule_feedthrough, internal_top_wire
// CLUE: same bracket with NO part select in the sub (whole-bus feedthrough): isolates whether slicing or the internal top wire is what breaks the flat write.

module sub (input [1:0] a, output [1:0] y);
  assign y = a;
endmodule

module top (input [1:0] i, input zi, output [1:0] o, output zo);
  wire [1:0] m;
  sub u0 (.a(i), .y(m));
  assign o = m;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
