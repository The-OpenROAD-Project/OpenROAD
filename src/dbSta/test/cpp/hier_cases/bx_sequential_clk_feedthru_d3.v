// TOP: top
// TECH: nangate45
// TARGETS: clock_feedthrough_depth_3, pure_feedthrough_ports
// CLUE: the clock is distributed down three hierarchy levels purely by port
// feedthrough (no buffer anywhere); flattening must not lose the clock net.

module leaf (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module l2 (input d, input ck, output q);
  leaf c (.d(d), .ck(ck), .q(q));
endmodule

module l1 (input d, input ck, output q);
  l2 c (.d(d), .ck(ck), .q(q));
endmodule

module top (input d, input ck, output q);
  l1 u (.d(d), .ck(ck), .q(q));
endmodule
