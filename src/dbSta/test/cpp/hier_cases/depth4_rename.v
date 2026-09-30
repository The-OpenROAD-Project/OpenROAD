// TOP: top
// TECH: nangate45
// TARGETS: depth_4, feedthrough_rename
// CLUE: feedthrough renamed at EVERY level of a depth-4 chain; each level a new port and net name.

module s4 (input d4, output q4);
  assign q4 = d4;
endmodule

module s3 (input d3, output q3);
  wire n3;
  s4 u4 (.d4(d3), .q4(n3));
  assign q3 = n3;
endmodule

module s2 (input d2, output q2);
  wire n2;
  s3 u3 (.d3(d2), .q3(n2));
  assign q2 = n2;
endmodule

module s1 (input d1, output q1);
  wire n1;
  s2 u2 (.d2(d1), .q2(n1));
  assign q1 = n1;
endmodule

module top (input i, input zi, output o, output zo);
  wire n0;
  s1 u1 (.d1(i), .q1(n0));
  assign o = n0;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
