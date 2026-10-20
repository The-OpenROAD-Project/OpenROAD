// TOP: top
// TECH: nangate45
// TARGETS: d_tied_by_logic0_cell, constant_cell_not_literal
// CLUE: D is tied low by a LOGIC0_X1 instance (not a 1'b0 literal) inside the
// submodule, so the tie must survive as a cell through the round trip.

module sub (input ck, output q, output qn);
  wire zero;
  LOGIC0_X1 t (.Z(zero));
  DFF_X1 ff (.D(zero), .CK(ck), .Q(q), .QN(qn));
endmodule

module top (input ck, output q, output qn);
  sub u (.ck(ck), .q(q), .qn(qn));
endmodule
