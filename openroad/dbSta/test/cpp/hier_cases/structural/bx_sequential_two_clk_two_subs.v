// TOP: top
// TECH: nangate45
// TARGETS: two_independent_clocks, one_domain_per_submodule
// CLUE: two unrelated top clocks, each feeding its own 2-flop group in its own
// submodule; a writer that merges or mis-orders clock nets would cross domains.

module domA (input d, input ck, output q);
  wire m;
  DFF_X1 f0 (.D(d), .CK(ck), .Q(m));
  DFF_X1 f1 (.D(m), .CK(ck), .Q(q));
endmodule

module domB (input d, input ck, output q);
  wire m;
  DFF_X1 f0 (.D(d), .CK(ck), .Q(m));
  DFF_X1 f1 (.D(m), .CK(ck), .Q(q));
endmodule

module top (input da, input db, input cka, input ckb, output za, output zb);
  domA ua (.d(da), .ck(cka), .q(za));
  domB ub (.d(db), .ck(ckb), .q(zb));
endmodule
