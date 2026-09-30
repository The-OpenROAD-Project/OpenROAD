// TOP: top
// TECH: nangate45
// TARGETS: one_clock_two_sibling_modules, clock_fanout
// CLUE: a single top clock fans out to flops in two sibling submodules; the
// two domain outputs are recombined at the parent.

module domA (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module domB (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module top (input a, input b, input ck, output z);
  wire qa, qb;
  domA ua (.d(a), .ck(ck), .q(qa));
  domB ub (.d(b), .ck(ck), .q(qb));
  XOR2_X1 g (.A(qa), .B(qb), .Z(z));
endmodule
