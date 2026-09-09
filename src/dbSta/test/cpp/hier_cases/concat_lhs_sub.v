// TOP: top
// TECH: nangate45
// TARGETS: concat_lhs, submodule_feedthrough
// CLUE: concatenation on the LHS of an assign inside a submodule: {y[0],y[1]} bit-reverses; legal Verilog-2005, probes the reader.

module sub (input [1:0] a, output [1:0] y);
  assign {y[0], y[1]} = a;
endmodule

module top (input [1:0] i, input zi, output [1:0] o, output zo);
  sub u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
