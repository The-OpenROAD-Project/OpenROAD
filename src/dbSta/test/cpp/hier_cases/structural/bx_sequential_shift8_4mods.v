// TOP: top
// TECH: nangate45
// TARGETS: shift_register_8_deep, 4_sibling_modules, 2_dff_per_module
// CLUE: an 8-deep shift register split as 4 sibling instances of a 2-flop
// module: every other stage-to-stage hop is a module boundary crossing.

module pair (input d, input ck, output q);
  wire m;
  DFF_X1 f0 (.D(d), .CK(ck), .Q(m));
  DFF_X1 f1 (.D(m), .CK(ck), .Q(q));
endmodule

module top (input d, input ck, output q);
  wire s1, s2, s3;
  pair p0 (.d(d),  .ck(ck), .q(s1));
  pair p1 (.d(s1), .ck(ck), .q(s2));
  pair p2 (.d(s2), .ck(ck), .q(s3));
  pair p3 (.d(s3), .ck(ck), .q(q));
endmodule
