// TOP: top
// TECH: nangate45
// TARGETS: probe_cell_dffr, async_reset
// CLUE: probes whether the SEC oracle models DFFR_X1 (active-low async reset).
// If self-check fails the cell is unusable for LEC in this campaign.

module sub (input d, input ck, input rn, output q, output qn);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q), .QN(qn));
endmodule

module top (input d, input ck, input rn, output q, output qn);
  sub u (.d(d), .ck(ck), .rn(rn), .q(q), .qn(qn));
endmodule
