// TOP: top
// TECH: nangate45
// TARGETS: shift_register_8_deep, nested_depth_4, 2_dff_per_level
// CLUE: same 8-deep register but the modules are NESTED 4 levels instead of
// being siblings; the register path descends and returns at every level.

module l4 (input d, input ck, output q);
  wire m;
  DFF_X1 a (.D(d), .CK(ck), .Q(m));
  DFF_X1 b (.D(m), .CK(ck), .Q(q));
endmodule

module l3 (input d, input ck, output q);
  wire m, n;
  DFF_X1 a (.D(d), .CK(ck), .Q(m));
  DFF_X1 b (.D(m), .CK(ck), .Q(n));
  l4 c (.d(n), .ck(ck), .q(q));
endmodule

module l2 (input d, input ck, output q);
  wire m, n;
  DFF_X1 a (.D(d), .CK(ck), .Q(m));
  DFF_X1 b (.D(m), .CK(ck), .Q(n));
  l3 c (.d(n), .ck(ck), .q(q));
endmodule

module l1 (input d, input ck, output q);
  wire m, n;
  DFF_X1 a (.D(d), .CK(ck), .Q(m));
  DFF_X1 b (.D(m), .CK(ck), .Q(n));
  l2 c (.d(n), .ck(ck), .q(q));
endmodule

module top (input d, input ck, output q);
  l1 u (.d(d), .ck(ck), .q(q));
endmodule
