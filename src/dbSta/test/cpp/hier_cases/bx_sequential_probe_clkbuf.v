// TOP: top
// TECH: nangate45
// TARGETS: probe_cell_clkbuf, clock_buffer
// CLUE: probes CLKBUF_X1 on the clock path of a DFF inside a submodule.

module sub (input ck, input d, output q);
  wire ckb;
  CLKBUF_X1 cb (.A(ck), .Z(ckb));
  DFF_X1 ff (.D(d), .CK(ckb), .Q(q));
endmodule

module top (input ck, input d, output q);
  sub u (.ck(ck), .d(d), .q(q));
endmodule
