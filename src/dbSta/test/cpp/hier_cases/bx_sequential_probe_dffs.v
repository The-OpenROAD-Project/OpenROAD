// TOP: top
// TECH: nangate45
// TARGETS: probe_cell_dffs, async_set
// CLUE: probes whether the SEC oracle models DFFS_X1 (active-low async set).

module sub (input d, input ck, input sn, output q, output qn);
  DFFS_X1 ff (.D(d), .SN(sn), .CK(ck), .Q(q), .QN(qn));
endmodule

module top (input d, input ck, input sn, output q, output qn);
  sub u (.d(d), .ck(ck), .sn(sn), .q(q), .qn(qn));
endmodule
