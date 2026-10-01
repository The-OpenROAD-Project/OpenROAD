// TOP: top
// TECH: nangate45
// TARGETS: two_independent_clocks, gated_clock_x1_per_submodule
// CLUE: each domain has its own gated clock inside its
// submodule; the ICG is a sequential cell sitting on the clock path.
// NOTE: the clock gate is an AND2_X1, not a CLKGATE_X1. The hazard here is
// the gated clock crossing module boundaries, which AND2 exercises identically;
// the ICG cell itself is unmodellable by the oracle (arity mismatch) and is
// probed deliberately by bx_sequential_probe_clkgate.v instead.

module domA (input d, input ck, input e, output q);
  wire gck;
  AND2_X1 icg (.A1(ck), .A2(e), .ZN(gck));
  DFF_X1 ff (.D(d), .CK(gck), .Q(q));
endmodule

module domB (input d, input ck, input e, output q);
  wire gck;
  AND2_X1 icg (.A1(ck), .A2(e), .ZN(gck));
  DFF_X1 ff (.D(d), .CK(gck), .Q(q));
endmodule

module top (input da, input db, input cka, input ckb, input ea, input eb,
            output za, output zb);
  domA ua (.d(da), .ck(cka), .e(ea), .q(za));
  domB ub (.d(db), .ck(ckb), .e(eb), .q(zb));
endmodule
