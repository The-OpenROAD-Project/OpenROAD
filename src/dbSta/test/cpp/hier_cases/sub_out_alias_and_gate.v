// TOP: top
// TECH: nangate45
// TARGETS: feedthrough_plus_external_load
// CLUE: a feedthrough submodule output drives BOTH a top output port directly and a top gate, so the dissolved alias net keeps two kinds of load.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, input zi, output o1, output o2, output zo);
  wire m;
  ft u0 (.a(i), .y(m));
  assign o1 = m;
  INV_X1 g0 (.A(m), .ZN(o2));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
