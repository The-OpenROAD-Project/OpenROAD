// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, sliced_instance_input, direct_port_out
// CLUE: control for slicedin_bus_topwire: same narrowing input slice but the sub output goes straight to a slice of the top output PORT (no internal wire).

module sub (input [3:0] a, output [1:0] y);
  assign y = a[3:2];
endmodule

module top (input [7:0] i, output [3:0] o);
  sub u0 (.a(i[3:0]), .y(o[1:0]));
  INV_X1 g0 (.A(i[7]), .ZN(o[2]));
  INV_X1 g1 (.A(i[6]), .ZN(o[3]));
endmodule
