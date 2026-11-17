// TOP: top
// TECH: nangate45
// TARGETS: inverted_clock_in_submodule, negedge_domain
// CLUE: the clock is inverted inside a submodule, creating a negedge flop
// group; a writer that reorders or re-buffers the clock could break polarity.

module negdom (input d, input ck, output q);
  wire ckn;
  INV_X1 ci (.A(ck), .ZN(ckn));
  DFF_X1 ff (.D(d), .CK(ckn), .Q(q));
endmodule

module posdom (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module top (input d, input ck, output zn, output zp);
  negdom un (.d(d), .ck(ck), .q(zn));
  posdom up (.d(d), .ck(ck), .q(zp));
endmodule
