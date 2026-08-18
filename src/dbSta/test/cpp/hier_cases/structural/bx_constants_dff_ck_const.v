// TOP: top
// TECH: nangate45
// TARGETS: tie0, dff_clock_pin, probe_oracle
// CLUE: constant 1'b0 on a DFF CLOCK pin — flop never toggles; oracle may
// refuse (X state forever). If self-check fails tag unusable but record the
// emitted structure for the CK tie.
module top (input a, input ck_unused, output q, output y);
  DFF_X1 f (.D(a), .CK(1'b0), .Q(q));
  BUF_X1 b (.A(ck_unused), .Z(y));
endmodule
