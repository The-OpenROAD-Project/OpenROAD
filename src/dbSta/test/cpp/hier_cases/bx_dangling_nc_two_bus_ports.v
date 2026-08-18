// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, two_open_bus_ports, nc_filler_numbering
// CLUE: one instance leaves TWO different bus inputs open. Filler numbering must
// not reuse names between the two concatenations.
module sub (input a, input [1:0] db, input [1:0] eb, output y);
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module top (input x, output y);
  sub u0 (.a(x), .y(y));
endmodule
