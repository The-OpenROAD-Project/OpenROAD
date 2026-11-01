// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, submodule_feedthrough
// CLUE: minimal known-finding-2 shape: submodule assign out_bus = in_bus[3:2]; flat write historically drops it leaving top outputs undriven.

module sub (input [3:0] in_bus, output [1:0] out_bus);
  assign out_bus = in_bus[3:2];
endmodule

module top (input [3:0] i, input zi, output [1:0] o, output zo);
  sub u0 (.in_bus(i), .out_bus(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
