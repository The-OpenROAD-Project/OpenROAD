// TOP: top
// TECH: nangate45
// TARGETS: delta_debug, feedthrough_wire_two_readers
// CLUE: minimal shape: a SCALAR feedthrough wire from a sub read by two separate top assigns to two output ports.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, output o1, output o2);
  wire m;
  ft u0 (.a(i), .y(m));
  assign o1 = m;
  assign o2 = m;
endmodule
