// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, partial_bus_drive, submodule
// CLUE: half of the submodule output bus comes from a slice feedthrough assign and half from gates: a dropped assign leaves exactly two bits undriven.

module sub (input [3:0] a, output [3:0] y);
  assign y[3:2] = a[1:0];
  INV_X1 g0 (.A(a[2]), .ZN(y[1]));
  INV_X1 g1 (.A(a[3]), .ZN(y[0]));
endmodule

module top (input [3:0] i, output [3:0] o);
  sub u0 (.a(i), .y(o));
endmodule
