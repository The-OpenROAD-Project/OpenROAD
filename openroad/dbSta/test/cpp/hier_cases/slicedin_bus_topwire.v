// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, sliced_instance_input, internal_top_wire
// CLUE: suspected finding-2 trigger isolated: sub bus-slice feedthrough whose instance INPUT is a narrowing slice of a wider top bus, output on an internal top wire.

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [7:0] i, input zi, output [1:0] o, output zo);
  wire [1:0] m;
  sub u0 (.a(i[3:0]), .y(m));
  assign o = m;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
