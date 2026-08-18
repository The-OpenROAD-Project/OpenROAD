// TOP: top
// TECH: nangate45
// TARGETS: concat_read_of_feedthrough_wire, mixed_concat
// CLUE: suspect T2 with the get_ports1 flavour: the top concat mixes a direct top input bit with bits of the sub's feedthrough output wire.

module ft (input [1:0] a, output [1:0] y);
  assign y = a;
endmodule

module top (input [1:0] i, input k, output [2:0] o);
  wire [1:0] m;
  ft u0 (.a(i), .y(m));
  assign o = {k, m[1], m[0]};
endmodule
