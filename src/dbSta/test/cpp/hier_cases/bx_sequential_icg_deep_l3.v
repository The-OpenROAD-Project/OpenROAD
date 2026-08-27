// TOP: top
// TECH: nangate45
// TARGETS: gated_clock_in_depth_3_leaf, enable_fed_through
// CLUE: the gated clock lives in the deepest of three nested
// modules; both the raw clock and the enable are fed through every level.
// NOTE: the clock gate is an AND2_X1, not a CLKGATE_X1. The hazard here is
// the gated clock crossing module boundaries, which AND2 exercises identically;
// the ICG cell itself is unmodellable by the oracle (arity mismatch) and is
// probed deliberately by bx_sequential_probe_clkgate.v instead.

module leaf (input d, input ck, input e, output q);
  wire gck;
  AND2_X1 icg (.A1(ck), .A2(e), .ZN(gck));
  DFF_X1 ff (.D(d), .CK(gck), .Q(q));
endmodule

module l2 (input d, input ck, input e, output q);
  leaf c (.d(d), .ck(ck), .e(e), .q(q));
endmodule

module l1 (input d, input ck, input e, output q);
  l2 c (.d(d), .ck(ck), .e(e), .q(q));
endmodule

module top (input d, input ck, input e, output q);
  l1 u (.d(d), .ck(ck), .e(e), .q(q));
endmodule
