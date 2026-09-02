// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, submodule_feedthrough, internal_top_wire
// CLUE: bracket for the finding-2 trigger: identical sub to busslice_leaf, but the sub bus output lands on an internal top WIRE that a top assign forwards to the port.

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [3:0] i, input zi, output [1:0] o, output zo);
  wire [1:0] m;
  sub u0 (.a(i), .y(m));
  assign o = m;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
