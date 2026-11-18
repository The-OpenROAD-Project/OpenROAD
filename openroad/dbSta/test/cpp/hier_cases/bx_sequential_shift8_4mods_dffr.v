// TOP: top
// TECH: nangate45
// TARGETS: shift_register_8_deep, 4_sibling_modules, dffr_reset_anchored
// CLUE: LEC-usable twin of shift8_4mods: same 8-deep register split over 4
// sibling modules, but with DFFR_X1 so the oracle can anchor the state.

module pair (input d, input ck, input rn, output q);
  wire m;
  DFFR_X1 f0 (.D(d), .RN(rn), .CK(ck), .Q(m));
  DFFR_X1 f1 (.D(m), .RN(rn), .CK(ck), .Q(q));
endmodule

module top (input d, input ck, input rn, output q);
  wire s1, s2, s3;
  pair p0 (.d(d),  .ck(ck), .rn(rn), .q(s1));
  pair p1 (.d(s1), .ck(ck), .rn(rn), .q(s2));
  pair p2 (.d(s2), .ck(ck), .rn(rn), .q(s3));
  pair p3 (.d(s3), .ck(ck), .rn(rn), .q(q));
endmodule
