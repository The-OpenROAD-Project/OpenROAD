// TOP: top
// TECH: nangate45
// TARGETS: feedthrough_chain, assign_at_every_level
// CLUE: 4-level chain where EVERY level inserts wire-to-wire assigns before
// and after its child instance; multiplies the boundary-assign hazard that
// dropped feedthrough assigns in the known flat finding.

module gleaf (input i, output o);
  assign o = i;
endmodule

module g3 (input i, output o);
  wire ti, to;
  assign ti = i;
  gleaf u (.i(ti), .o(to));
  assign o = to;
endmodule

module g2 (input i, output o);
  wire ti, to;
  assign ti = i;
  g3 u (.i(ti), .o(to));
  assign o = to;
endmodule

module g1 (input i, output o);
  wire ti, to;
  assign ti = i;
  g2 u (.i(ti), .o(to));
  assign o = to;
endmodule

module top (input a, output z);
  g1 u (.i(a), .o(z));
endmodule
