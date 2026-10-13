// TOP: top
// TECH: nangate45
// TARGETS: concat_lhs, top_level
// CLUE: concat LHS assign at top level (control for concat_lhs_sub).

module top (input [1:0] i, input zi, output [1:0] o, output zo);
  assign {o[0], o[1]} = i;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
