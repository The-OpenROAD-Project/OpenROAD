// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, two_instances, submodule
// CLUE: two instances of the same bus-slice feedthrough submodule; flat write must create two independent alias routes from one module definition.

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [3:0] i, input [3:0] j, input zi,
            output [1:0] o1, output [1:0] o2, output zo);
  sub u0 (.a(i), .y(o1));
  sub u1 (.a(j), .y(o2));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
