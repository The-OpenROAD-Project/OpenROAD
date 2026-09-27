// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, internal_top_wire, wider_out_bus
// CLUE: sub bus-slice feedthrough onto an internal top wire that is assigned into a SLICE of a wider top output bus whose other bits come from gates.

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [3:0] i, output [3:0] o);
  wire [1:0] m;
  sub u0 (.a(i), .y(m));
  assign o[1:0] = m;
  INV_X1 g0 (.A(i[0]), .ZN(o[2]));
  INV_X1 g1 (.A(i[1]), .ZN(o[3]));
endmodule
