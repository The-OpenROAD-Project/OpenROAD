// TOP: top
// TECH: nangate45
// TARGETS: alias_output_from_output, submodule, bus
// CLUE: submodule bus output assigned from another submodule bus output (both are output ports of the child).

module oo (input [1:0] a, output [1:0] y1, output [1:0] y2);
  INV_X1 g0 (.A(a[0]), .ZN(y1[0]));
  INV_X1 g1 (.A(a[1]), .ZN(y1[1]));
  assign y2 = y1;
endmodule

module top (input [1:0] i, output [1:0] o1, output [1:0] o2);
  oo u0 (.a(i), .y1(o1), .y2(o2));
endmodule
