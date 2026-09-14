// TOP: top
// TECH: nangate45
// TARGETS: alias_both_names_loaded, bus
// CLUE: bus net and its whole-bus assign-alias both feed gate pins; collapse must keep every bit's loads.

module top (input [1:0] i, output [1:0] o1, output [1:0] o2);
  wire [1:0] a, b;
  INV_X1 g0 (.A(i[0]), .ZN(a[0]));
  INV_X1 g1 (.A(i[1]), .ZN(a[1]));
  assign b = a;
  INV_X1 g2 (.A(a[0]), .ZN(o1[0]));
  INV_X1 g3 (.A(a[1]), .ZN(o1[1]));
  INV_X1 g4 (.A(b[0]), .ZN(o2[0]));
  INV_X1 g5 (.A(b[1]), .ZN(o2[1]));
endmodule
