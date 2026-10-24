// TOP: top
// TECH: nangate45
// TARGETS: gated_clock_at_top, gated_clock_into_two_submodules
// CLUE: one clock gate at the top level drives the clock ports of two
// submodules; the gated clock net has hierarchical fanout.
// NOTE: the clock gate is an AND2_X1, not a CLKGATE_X1. The hazard here is
// the gated clock crossing module boundaries, which AND2 exercises identically;
// the ICG cell itself is unmodellable by the oracle (arity mismatch) and is
// probed deliberately by bx_sequential_probe_clkgate.v instead.

module domA (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module domB (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module top (input da, input db, input ck, input e, output za, output zb);
  wire gck;
  AND2_X1 icg (.A1(ck), .A2(e), .ZN(gck));
  domA ua (.d(da), .ck(gck), .q(za));
  domB ub (.d(db), .ck(gck), .q(zb));
endmodule
