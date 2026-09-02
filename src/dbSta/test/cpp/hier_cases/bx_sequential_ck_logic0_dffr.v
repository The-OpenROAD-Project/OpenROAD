// TOP: top
// TECH: nangate45
// TARGETS: clock_tied_by_logic0_cell, never_clocked_flop
// CLUE: the flop's CLOCK is tied low by a LOGIC0_X1 cell, so it never clocks
// and only the async reset defines its value -- a degenerate clock net that a
// writer might optimise away.

module sub (input d, input rn, output q, output qn);
  wire zero;
  LOGIC0_X1 t (.Z(zero));
  DFFR_X1 ff (.D(d), .RN(rn), .CK(zero), .Q(q), .QN(qn));
endmodule

module top (input d, input rn, output q, output qn);
  sub u (.d(d), .rn(rn), .q(q), .qn(qn));
endmodule
