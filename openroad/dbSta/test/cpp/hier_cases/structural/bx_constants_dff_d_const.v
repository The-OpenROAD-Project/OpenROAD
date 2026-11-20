// TOP: top
// TECH: nangate45
// TARGETS: tie1, dff_data_pin
// CLUE: constant 1'b1 on a DFF data pin — sequential cell with constant data;
// writer must keep the tie on D.
module top (input ck, output q, output qn);
  DFF_X1 f (.D(1'b1), .CK(ck), .Q(q), .QN(qn));
endmodule
