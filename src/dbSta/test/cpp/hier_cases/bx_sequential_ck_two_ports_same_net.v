// TOP: top
// TECH: nangate45
// TARGETS: two_clock_ports_tied_to_one_parent_net
// CLUE: the submodule declares two separate clock ports and the parent ties
// both to the same clock net; inside, the flops are split between them.

module twoclk (input d, input ck0, input ck1, output q0, output q1);
  DFF_X1 f0 (.D(d), .CK(ck0), .Q(q0));
  DFF_X1 f1 (.D(d), .CK(ck1), .Q(q1));
endmodule

module top (input d, input ck, output z0, output z1);
  twoclk u (.d(d), .ck0(ck), .ck1(ck), .q0(z0), .q1(z1));
endmodule
