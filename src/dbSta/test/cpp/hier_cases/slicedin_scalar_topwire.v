// TOP: top
// TECH: nangate45
// TARGETS: scalar_feedthrough, bitsel_instance_input, internal_top_wire
// CLUE: SCALAR feedthrough sub whose instance input is a single BIT of a top bus and whose output lands on an internal top wire: no buses inside the child at all.

module sub (input a, output y);
  assign y = a;
endmodule

module top (input [3:0] i, input zi, output o, output zo);
  wire m;
  sub u0 (.a(i[2]), .y(m));
  assign o = m;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
