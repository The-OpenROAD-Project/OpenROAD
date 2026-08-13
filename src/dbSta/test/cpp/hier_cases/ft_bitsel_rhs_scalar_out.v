// TOP: top
// TECH: nangate45
// TARGETS: bit_select_rhs, scalar_out, internal_top_wire
// CLUE: sub feedthrough with a bit-select RHS but a SCALAR output port, landing on an internal scalar top wire: bus on the read side only.

module sub (input [3:0] a, output y);
  assign y = a[2];
endmodule

module top (input [3:0] i, input zi, output o, output zo);
  wire m;
  sub u0 (.a(i), .y(m));
  assign o = m;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
