// TOP: top
// TECH: nangate45
// TARGETS: clock_generated_in_leaf, clock_exported_upward_then_sideways
// CLUE: the buffered clock is produced inside one submodule, exported up to
// the parent, and consumed by a flop in a SIBLING submodule.

module clkgen (input ck, output ckq);
  CLKBUF_X1 cb (.A(ck), .Z(ckq));
endmodule

module ffmod (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module top (input d, input ck, output q);
  wire ckb;
  clkgen g (.ck(ck), .ckq(ckb));
  ffmod u (.d(d), .ck(ckb), .q(q));
endmodule
