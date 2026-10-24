// TOP: top
// TECH: nangate45
// TARGETS: qn_only_used, q_pin_unconnected
// CLUE: only QN is used; the Q output pin of the flop is left unconnected in
// the submodule. LEC cannot see whether the dangling pin survives the trip.

module sub (input d, input ck, output qn);
  DFF_X1 ff (.D(d), .CK(ck), .QN(qn));
endmodule

module top (input d, input ck, output zn);
  sub u (.d(d), .ck(ck), .qn(zn));
endmodule
