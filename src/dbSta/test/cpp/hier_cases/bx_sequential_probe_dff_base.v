// TOP: top
// TECH: nangate45
// TARGETS: probe_cell_dff, baseline_hier_depth_1
// CLUE: baseline sanity: DFF_X1 inside one submodule, Q and QN both to top.
// If this does not pass, nothing else in the family is interpretable.

module sub (input d, input ck, output q, output qn);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q), .QN(qn));
endmodule

module top (input d, input ck, output q, output qn);
  sub u (.d(d), .ck(ck), .q(q), .qn(qn));
endmodule
