// TOP: top
// TECH: nangate45
// TARGETS: probe_cell_dlh, level_sensitive_latch
// CLUE: probes whether the SEC oracle models the DLH_X1 level-sensitive latch
// (transparent when G=1). Latches are the classic LEC modelling gap.

module sub (input d, input g, output q);
  DLH_X1 lat (.D(d), .G(g), .Q(q));
endmodule

module top (input d, input g, output q);
  sub u (.d(d), .g(g), .q(q));
endmodule
