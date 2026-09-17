// TOP: top
// TECH: nangate45
// TARGETS: depth_2, feedthrough_rename
// CLUE: feedthrough renamed at each of 2 hierarchy levels (every level a new net name).

module r2 (input d2, output q2);
  assign q2 = d2;
endmodule

module r1 (input d1, output q1);
  wire n1;
  r2 u2 (.d2(d1), .q2(n1));
  assign q1 = n1;
endmodule

module top (input i, input zi, output o, output zo);
  wire n0;
  r1 u1 (.d1(i), .q1(n0));
  assign o = n0;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
