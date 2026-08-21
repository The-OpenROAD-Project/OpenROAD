// TOP: top
// TECH: nangate45
// TARGETS: delta_debug, feedthrough_bus_wire_two_readers
// CLUE: minimal shape: a BUS feedthrough wire from a sub read whole by two separate top assigns to two output buses.

module ft (input [1:0] a, output [1:0] y);
  assign y = a;
endmodule

module top (input [1:0] i, output [1:0] o1, output [1:0] o2);
  wire [1:0] m;
  ft u0 (.a(i), .y(m));
  assign o1 = m;
  assign o2 = m;
endmodule
