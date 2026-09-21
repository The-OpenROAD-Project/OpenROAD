// TOP: top
// TECH: nangate45
// TARGETS: q_drives_top_output_and_internal_cone, dual_fanout
// CLUE: the flop's Q both leaves the submodule straight to a top output AND
// feeds an internal cone whose result is a second top output.

module sub (input d, input ck, input c, output q, output z);
  wire qi;
  DFF_X1 ff (.D(d), .CK(ck), .Q(qi));
  NAND2_X1 g (.A1(qi), .A2(c), .ZN(z));
  assign q = qi;
endmodule

module top (input d, input ck, input c, output q, output z);
  sub u (.d(d), .ck(ck), .c(c), .q(q), .z(z));
endmodule
