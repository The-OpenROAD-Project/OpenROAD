// TOP: top
// TECH: nangate45
// TARGETS: d_tied_by_logic1_cell, constant_cell_not_literal
// CLUE: D tied high by a LOGIC1_X1 instance inside the submodule; the twin of
// d_logic0_cell, to separate polarity bugs from tie-cell bugs.

module sub (input ck, output q, output qn);
  wire one;
  LOGIC1_X1 t (.Z(one));
  DFF_X1 ff (.D(one), .CK(ck), .Q(q), .QN(qn));
endmodule

module top (input ck, output q, output qn);
  sub u (.ck(ck), .q(q), .qn(qn));
endmodule
