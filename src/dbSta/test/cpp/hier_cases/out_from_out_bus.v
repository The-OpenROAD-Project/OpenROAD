// TOP: top
// TECH: nangate45
// TARGETS: alias_output_from_output, bus, top_level
// CLUE: whole-bus output port assigned FROM another whole-bus output port at top level.

module top (input [1:0] i, output [1:0] o1, output [1:0] o2);
  INV_X1 g0 (.A(i[0]), .ZN(o1[0]));
  INV_X1 g1 (.A(i[1]), .ZN(o1[1]));
  assign o2 = o1;
endmodule
