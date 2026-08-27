// TOP: top
// TECH: nangate45
// TARGETS: dff_qn_dangling
// CLUE: DFF whose Q is consumed but QN is omitted — half-dangling flop output.
module top (input in1, input ck, output out1);
  DFF_X1 ff1 (.D(in1), .CK(ck), .Q(out1));
endmodule
