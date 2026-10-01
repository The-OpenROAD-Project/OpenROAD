// TOP: top
// TECH: nangate45
// TARGETS: clock_feedthrough_depth_3, clkbuf_at_middle_level
// CLUE: a CLKBUF_X1 sits in the middle module of a 3-level clock path, so the
// clock net changes identity halfway down the hierarchy.

module leaf (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module l2 (input d, input ck, output q);
  wire ckb;
  CLKBUF_X1 cb (.A(ck), .Z(ckb));
  leaf c (.d(d), .ck(ckb), .q(q));
endmodule

module l1 (input d, input ck, output q);
  l2 c (.d(d), .ck(ck), .q(q));
endmodule

module top (input d, input ck, output q);
  l1 u (.d(d), .ck(ck), .q(q));
endmodule
