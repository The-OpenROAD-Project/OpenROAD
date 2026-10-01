// TOP: top
// TECH: nangate45
// TARGETS: dff_chain_two_modules, inversion_at_parent
// CLUE: flop A's Q leaves its module, is inverted at the parent, and enters
// flop B in a different module -- the sequential path goes up then down.

module ffa (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module ffb (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module top (input a, input ck, output z);
  wire q1, n1;
  ffa u1 (.d(a), .ck(ck), .q(q1));
  INV_X1 i0 (.A(q1), .ZN(n1));
  ffb u2 (.d(n1), .ck(ck), .q(z));
endmodule
