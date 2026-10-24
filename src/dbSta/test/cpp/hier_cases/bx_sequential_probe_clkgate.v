// TOP: top
// TECH: nangate45
// TARGETS: probe_cell_clkgate, integrated_clock_gate
// CLUE: probes whether the SEC oracle models CLKGATE_X1 (latch-based ICG).
// GCK drives a DFF clock, so the gate is on a real clock path.

module sub (input ck, input e, input d, output q);
  wire gck;
  CLKGATE_X1 icg (.CK(ck), .E(e), .GCK(gck));
  DFF_X1 ff (.D(d), .CK(gck), .Q(q));
endmodule

module top (input ck, input e, input d, output q);
  sub u (.ck(ck), .e(e), .d(d), .q(q));
endmodule
