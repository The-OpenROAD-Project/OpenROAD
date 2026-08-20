// TOP: top
// TECH: nangate45
// TARGETS: escaped_name_net, submodule_feedthrough
// CLUE: escaped-name net inside a submodule feedthrough chain; flat write must uniquify it into a legal (still escaped) flat name.

module ft (input a, output y);
  wire \mid#x ;
  assign \mid#x  = a;
  assign y = \mid#x ;
endmodule

module top (input i, input zi, output o, output zo);
  ft u0 (.a(i), .y(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
