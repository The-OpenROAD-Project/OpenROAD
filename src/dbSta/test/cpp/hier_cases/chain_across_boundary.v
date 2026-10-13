// TOP: top
// TECH: nangate45
// TARGETS: assign_chain_3, boundary_straddle
// CLUE: assign chain split across a boundary: top assign feeds the sub input, sub has its own feedthrough assign, top assign takes the sub output to the output port.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input zi, output o, output zo);
  wire pre, post;
  assign pre = i;
  ft u0 (.a(pre), .y(post));
  assign o = post;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
