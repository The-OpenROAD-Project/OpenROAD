// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, bus_slice_assign, finding2_shape
// CLUE: known finding 2's bus-slice feedthrough shape with the receiving top wire named to sort before the driving top input: same failure, bus flavour.

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [3:0] i, output [1:0] o);
  wire [1:0] a_bus;
  sub u0 (.a(i), .y(a_bus));
  assign o = a_bus;
endmodule
