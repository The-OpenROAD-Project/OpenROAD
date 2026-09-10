// TOP: top
// TECH: nangate45
// TARGETS: probe_cell_sdff, scan_mux
// CLUE: probes whether the SEC oracle models SDFF_X1 (scan flop with SE/SI).
// Scan flops are the shape a post-DFT netlist round trip must survive.

module sub (input d, input ck, input se, input si, output q, output qn);
  SDFF_X1 ff (.D(d), .SE(se), .SI(si), .CK(ck), .Q(q), .QN(qn));
endmodule

module top (input d, input ck, input se, input si, output q, output qn);
  sub u (.d(d), .ck(ck), .se(se), .si(si), .q(q), .qn(qn));
endmodule
