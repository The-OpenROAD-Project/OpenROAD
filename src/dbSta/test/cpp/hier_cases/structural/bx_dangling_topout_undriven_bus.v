// TOP: top
// TECH: nangate45
// TARGETS: top_output_undriven, bus_port
// CLUE: 4-bit top output zb has no driver at all. Bus form of the undriven-top-
// output hazard: does the bus port survive with its declared range?
module top (input x, output y, output [3:0] zb);
  INV_X1 g1 (.A(x), .ZN(y));
endmodule
