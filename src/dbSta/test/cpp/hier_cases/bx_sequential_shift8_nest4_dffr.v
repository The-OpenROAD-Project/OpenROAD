// TOP: top
// TECH: nangate45
// TARGETS: shift_register_8_deep, nested_depth_4, dffr_reset_anchored
// CLUE: LEC-usable twin of shift8_nest4: 8 flops over 4 NESTED levels with
// clock and reset both fed through every boundary.

module l4 (input d, input ck, input rn, output q);
  wire m;
  DFFR_X1 a (.D(d), .RN(rn), .CK(ck), .Q(m));
  DFFR_X1 b (.D(m), .RN(rn), .CK(ck), .Q(q));
endmodule

module l3 (input d, input ck, input rn, output q);
  wire m, n;
  DFFR_X1 a (.D(d), .RN(rn), .CK(ck), .Q(m));
  DFFR_X1 b (.D(m), .RN(rn), .CK(ck), .Q(n));
  l4 c (.d(n), .ck(ck), .rn(rn), .q(q));
endmodule

module l2 (input d, input ck, input rn, output q);
  wire m, n;
  DFFR_X1 a (.D(d), .RN(rn), .CK(ck), .Q(m));
  DFFR_X1 b (.D(m), .RN(rn), .CK(ck), .Q(n));
  l3 c (.d(n), .ck(ck), .rn(rn), .q(q));
endmodule

module l1 (input d, input ck, input rn, output q);
  wire m, n;
  DFFR_X1 a (.D(d), .RN(rn), .CK(ck), .Q(m));
  DFFR_X1 b (.D(m), .RN(rn), .CK(ck), .Q(n));
  l2 c (.d(n), .ck(ck), .rn(rn), .q(q));
endmodule

module top (input d, input ck, input rn, output q);
  l1 u (.d(d), .ck(ck), .rn(rn), .q(q));
endmodule
