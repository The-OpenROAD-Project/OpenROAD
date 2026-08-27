// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, internal_top_wire, gate_consumer
// CLUE: sub bus-slice feedthrough onto an internal top wire that is consumed by GATES instead of a top assign: isolates the consumer kind.

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [3:0] i, output [1:0] o);
  wire [1:0] m;
  sub u0 (.a(i), .y(m));
  INV_X1 g0 (.A(m[0]), .ZN(o[0]));
  INV_X1 g1 (.A(m[1]), .ZN(o[1]));
endmodule
