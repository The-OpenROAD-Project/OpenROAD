// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, sliced_port_connections
// CLUE: finding-2 sub driven through SLICED port connections: sub input is a slice of the top input bus and sub output feeds a slice of the top output bus.

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [7:0] i, output [3:0] o);
  sub u0 (.a(i[3:0]), .y(o[1:0]));
  INV_X1 g0 (.A(i[7]), .ZN(o[2]));
  INV_X1 g1 (.A(i[6]), .ZN(o[3]));
endmodule
