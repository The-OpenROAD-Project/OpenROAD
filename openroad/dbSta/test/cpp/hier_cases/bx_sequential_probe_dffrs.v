// TOP: top
// TECH: nangate45
// TARGETS: probe_cell_dffrs, async_set_reset
// CLUE: probes whether the SEC oracle models DFFRS_X1 (both async set+reset).

module sub (input d, input ck, input rn, input sn, output q, output qn);
  DFFRS_X1 ff (.D(d), .RN(rn), .SN(sn), .CK(ck), .Q(q), .QN(qn));
endmodule

module top (input d, input ck, input rn, input sn, output q, output qn);
  sub u (.d(d), .ck(ck), .rn(rn), .sn(sn), .q(q), .qn(qn));
endmodule
