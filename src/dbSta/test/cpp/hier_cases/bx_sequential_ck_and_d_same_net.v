// TOP: top
// TECH: nangate45
// TARGETS: same_net_on_d_and_ck_of_one_flop
// CLUE: one top net drives both D and CK of the same flop across a boundary;
// the submodule has two ports tied to the same parent wire.

module ffmod (input d, input ck, output q, output qn);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q), .QN(qn));
endmodule

module top (input a, output q, output qn);
  ffmod u (.d(a), .ck(a), .q(q), .qn(qn));
endmodule
