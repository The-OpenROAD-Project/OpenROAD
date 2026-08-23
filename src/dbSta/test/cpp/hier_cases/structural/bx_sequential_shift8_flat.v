// TOP: top
// TECH: nangate45
// TARGETS: shift_register_8_deep, no_hierarchy
// CLUE: flat bracket control for shift8_4mods -- same 8-deep register with
// zero module boundaries, to attribute any failure to hierarchy vs depth.

module top (input d, input ck, output q);
  wire s1, s2, s3, s4, s5, s6, s7;
  DFF_X1 f0 (.D(d),  .CK(ck), .Q(s1));
  DFF_X1 f1 (.D(s1), .CK(ck), .Q(s2));
  DFF_X1 f2 (.D(s2), .CK(ck), .Q(s3));
  DFF_X1 f3 (.D(s3), .CK(ck), .Q(s4));
  DFF_X1 f4 (.D(s4), .CK(ck), .Q(s5));
  DFF_X1 f5 (.D(s5), .CK(ck), .Q(s6));
  DFF_X1 f6 (.D(s6), .CK(ck), .Q(s7));
  DFF_X1 f7 (.D(s7), .CK(ck), .Q(q));
endmodule
