// TOP: top
// TECH: nangate45
// TARGETS: dff_chain_two_modules, inversion_at_parent, dffr_reset_anchored
// CLUE: LEC-usable twin of chain_up_down: Q leaves module A, is inverted at
// the parent and enters module B's flop.

module ffa (input d, input ck, input rn, output q);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q));
endmodule

module ffb (input d, input ck, input rn, output q);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q));
endmodule

module top (input a, input ck, input rn, output z);
  wire q1, n1;
  ffa u1 (.d(a), .ck(ck), .rn(rn), .q(q1));
  INV_X1 i0 (.A(q1), .ZN(n1));
  ffb u2 (.d(n1), .ck(ck), .rn(rn), .q(z));
endmodule
