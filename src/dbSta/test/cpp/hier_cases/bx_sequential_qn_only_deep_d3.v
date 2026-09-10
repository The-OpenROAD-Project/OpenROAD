// TOP: top
// TECH: nangate45
// TARGETS: qn_only_used, dangling_q_at_depth_3
// CLUE: depth-3 bracket of the QN-only case: the unused Q pin is three levels
// down, and each intermediate module has no Q port at all.

module leaf (input d, input ck, output qn);
  DFF_X1 ff (.D(d), .CK(ck), .QN(qn));
endmodule

module l2 (input d, input ck, output qn);
  leaf c (.d(d), .ck(ck), .qn(qn));
endmodule

module l1 (input d, input ck, output qn);
  l2 c (.d(d), .ck(ck), .qn(qn));
endmodule

module top (input d, input ck, output zn);
  l1 u (.d(d), .ck(ck), .qn(zn));
endmodule
