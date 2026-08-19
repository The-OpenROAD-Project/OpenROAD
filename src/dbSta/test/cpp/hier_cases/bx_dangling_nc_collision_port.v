// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, nc_filler_vs_top_port
// CLUE: hier fills an unconnected bus port with invented wires _NC1.._NCn; here
// top already has an INPUT PORT named _NC1 driving live logic. A filler
// wire _NC1 would redeclare a port and steal its value.
module sub (input a, input [3:0] db, output y);
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module top (input x, input _NC1, output y);
  wire t;
  sub u0 (.a(x), .y(t));
  AND2_X1 g1 (.A1(t), .A2(_NC1), .ZN(y));
endmodule
