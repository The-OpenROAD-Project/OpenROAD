// TOP: top
// TECH: nangate45
// TARGETS: two_clocks_one_module, positional_port_connections
// CLUE: the two clocks enter a single submodule by POSITIONAL connection with
// the clock arguments given in the opposite order of the port list -- if the
// reader or writer normalises port order the domains swap silently.

module twodom (input ck0, input ck1, input d, output q0, output q1);
  DFF_X1 f0 (.D(d), .CK(ck0), .Q(q0));
  DFF_X1 f1 (.D(d), .CK(ck1), .Q(q1));
endmodule

module top (input cka, input ckb, input d, output za, output zb);
  twodom u (ckb, cka, d, za, zb);
endmodule
