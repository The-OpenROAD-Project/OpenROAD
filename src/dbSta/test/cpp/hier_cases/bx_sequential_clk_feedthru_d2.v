// TOP: top
// TECH: nangate45
// TARGETS: clock_feedthrough_depth_2
// CLUE: bracket for the depth-3 clock feedthrough: the clock reaches the flop
// through one intermediate module whose ck port only feeds its child.

module leaf (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module mid (input d, input ck, output q);
  leaf c (.d(d), .ck(ck), .q(q));
endmodule

module top (input d, input ck, output q);
  mid u (.d(d), .ck(ck), .q(q));
endmodule
