// TOP: top
// TECH: nangate45
// TARGETS: logic0_cell_in_one_module_dff_in_another
// CLUE: the tie cell and the flop it feeds live in DIFFERENT submodules, so
// the constant crosses two boundaries before reaching D.

module tie (output z);
  LOGIC0_X1 t (.Z(z));
endmodule

module ffmod (input d, input ck, output q, output qn);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q), .QN(qn));
endmodule

module top (input ck, output q, output qn);
  wire zero;
  tie u0 (.z(zero));
  ffmod u1 (.d(zero), .ck(ck), .q(q), .qn(qn));
endmodule
