// TOP: top
// TECH: nangate45
// TARGETS: reset_feedthrough_depth_3, clock_and_reset_both_fed_through
// CLUE: clock AND async reset are both fed down three levels of pure port
// feedthrough to a 2-flop leaf register.

module leaf (input d, input ck, input rn, output q);
  wire m;
  DFFR_X1 a (.D(d), .RN(rn), .CK(ck), .Q(m));
  DFFR_X1 b (.D(m), .RN(rn), .CK(ck), .Q(q));
endmodule

module l2 (input d, input ck, input rn, output q);
  leaf c (.d(d), .ck(ck), .rn(rn), .q(q));
endmodule

module l1 (input d, input ck, input rn, output q);
  l2 c (.d(d), .ck(ck), .rn(rn), .q(q));
endmodule

module top (input d, input ck, input rn, output q);
  l1 u (.d(d), .ck(ck), .rn(rn), .q(q));
endmodule
