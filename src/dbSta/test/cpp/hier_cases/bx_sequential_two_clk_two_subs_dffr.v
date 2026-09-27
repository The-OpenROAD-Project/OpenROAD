// TOP: top
// TECH: nangate45
// TARGETS: two_independent_clocks, two_flop_group_per_submodule, dffr
// CLUE: LEC-usable twin of two_clk_two_subs: two clock domains, each a 2-deep
// DFFR chain in its own submodule, sharing one asynchronous reset.

module domA (input d, input ck, input rn, output q);
  wire m;
  DFFR_X1 f0 (.D(d), .RN(rn), .CK(ck), .Q(m));
  DFFR_X1 f1 (.D(m), .RN(rn), .CK(ck), .Q(q));
endmodule

module domB (input d, input ck, input rn, output q);
  wire m;
  DFFR_X1 f0 (.D(d), .RN(rn), .CK(ck), .Q(m));
  DFFR_X1 f1 (.D(m), .RN(rn), .CK(ck), .Q(q));
endmodule

module top (input da, input db, input cka, input ckb, input rn,
            output za, output zb);
  domA ua (.d(da), .ck(cka), .rn(rn), .q(za));
  domB ub (.d(db), .ck(ckb), .rn(rn), .q(zb));
endmodule
