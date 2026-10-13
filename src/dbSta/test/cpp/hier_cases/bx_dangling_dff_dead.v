// TOP: top
// TECH: nangate45
// TARGETS: dead_dff, open_q_qn
// CLUE: DFF with D and CK driven but Q and QN both omitted — sequential dead element.
// Combinational live path keeps LEC usable.
module top (input in1, input ck, output out1);
  INV_X1 g1 (.A(in1), .ZN(out1));
  DFF_X1 ff1 (.D(in1), .CK(ck));
endmodule
