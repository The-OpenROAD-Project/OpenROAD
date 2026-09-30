// TOP: top
// TECH: nangate45
// TARGETS: q_of_one_module_is_clock_of_another, ripple_counter
// CLUE: a toggle flop in one submodule produces the CLOCK of a flop in another
// submodule -- a ripple clock whose source is a register output.

module ffa (input d, input ck, input rn, output q);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q));
endmodule

module ffb (input d, input ck, input rn, output q);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q));
endmodule

module top (input d, input ck, input rn, output z);
  wire q0, n0;
  ffa u0 (.d(n0), .ck(ck), .rn(rn), .q(q0));
  INV_X1 i0 (.A(q0), .ZN(n0));
  ffb u1 (.d(d), .ck(q0), .rn(rn), .q(z));
endmodule
