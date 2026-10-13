// TOP: top
// TECH: nangate45
// TARGETS: black_box, bus_bit_order, unknown_direction
// CLUE: makeBlackBoxNamedPorts (VerilogReader.cc:1800) invents a bus port as
// CLUE: makeBusPort(cell, name, 0, size-1) -- ASCENDING -- while the connected net is
// CLUE: iterated MSB-first, so a[3] lands on d[0]; every port also gets
// CLUE: PortDirection::unknown(), which Verilog2db turns into INPUT.
module top (a, y);
  input [3:0] a;
  output [3:0] y;
  ip_core u (.d(a), .q(y));
endmodule
