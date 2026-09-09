// TOP: top
// TECH: nangate45
// TARGETS: generated_clock_from_flop_and_clock, gate_at_parent
// CLUE: the clock of the second submodule is (raw clock AND the first
// submodule's Q) -- a generated clock built at the parent from a register.

module ffa (input d, input ck, input rn, output q);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q));
endmodule

module ffb (input d, input ck, input rn, output q);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q));
endmodule

module top (input d, input e, input ck, input rn, output z);
  wire q0, gck;
  ffa u0 (.d(e), .ck(ck), .rn(rn), .q(q0));
  AND2_X1 g (.A1(ck), .A2(q0), .ZN(gck));
  ffb u1 (.d(d), .ck(gck), .rn(rn), .q(z));
endmodule
