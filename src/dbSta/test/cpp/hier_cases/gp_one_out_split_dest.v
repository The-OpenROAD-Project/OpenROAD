// TOP: top
// TECH: nangate45
// TARGETS: delta_debug, split_destination_bus_and_scalar
// CLUE: minimal shape of the suspected trigger: ONE bus feedthrough wire whose bits are split between a top output BUS bit and a separate scalar top output port.

module ft (input [1:0] a, output [1:0] y);
  assign y = a;
endmodule

module top (input [1:0] i, input k, output [1:0] o, output p);
  wire [1:0] m;
  ft u0 (.a(i), .y(m));
  assign o = {k, m[1]};
  assign p = m[0];
endmodule
