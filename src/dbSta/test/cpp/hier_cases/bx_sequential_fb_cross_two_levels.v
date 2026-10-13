// TOP: top
// TECH: nangate45
// TARGETS: feedback_loop_crossing_two_levels, toggle_flop
// CLUE: deeper bracket of fb_cross_boundary: the loop leaves a depth-2 leaf,
// travels up through a feedthrough module, inverts at top, and comes back down.

module leaf (input d, input ck, input rn, output q);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q));
endmodule

module mid (input d, input ck, input rn, output q);
  leaf c (.d(d), .ck(ck), .rn(rn), .q(q));
endmodule

module top (input ck, input rn, output z);
  wire q, n;
  mid u (.d(n), .ck(ck), .rn(rn), .q(q));
  INV_X1 i0 (.A(q), .ZN(n));
  assign z = q;
endmodule
