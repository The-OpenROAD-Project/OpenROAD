// TOP: top
// TECH: nangate45
// TARGETS: two_independent_clocks, and_gated_clock_in_submodule
// CLUE: each domain gates its own clock with a plain AND2_X1 inside its
// submodule -- combinational logic on a clock net across a boundary.

module domA (input d, input ck, input en, output q);
  wire gck;
  AND2_X1 cg (.A1(ck), .A2(en), .ZN(gck));
  DFF_X1 ff (.D(d), .CK(gck), .Q(q));
endmodule

module domB (input d, input ck, input en, output q);
  wire gck;
  AND2_X1 cg (.A1(ck), .A2(en), .ZN(gck));
  DFF_X1 ff (.D(d), .CK(gck), .Q(q));
endmodule

module top (input da, input db, input cka, input ckb, input ea, input eb,
            output za, output zb);
  domA ua (.d(da), .ck(cka), .en(ea), .q(za));
  domB ub (.d(db), .ck(ckb), .en(eb), .q(zb));
endmodule
