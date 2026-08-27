// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, ascending_range, submodule_feedthrough
// CLUE: finding-2 shape with ASCENDING [0:N] bus ranges; range-direction handling in the writer.

module sub (input [0:3] a, output [0:1] y);
  assign y = a[2:3];
endmodule

module top (input [0:3] i, input zi, output [0:1] o, output zo);
  sub u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
