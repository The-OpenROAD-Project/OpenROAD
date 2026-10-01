// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, top_level
// CLUE: same bus-slice assign shape but at TOP level (control: no submodule involved).

module top (input [3:0] i, input zi, output [1:0] o, output zo);
  assign o = i[3:2];
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
