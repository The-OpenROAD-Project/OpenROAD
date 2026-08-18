// TOP: top
// TECH: nangate45
// TARGETS: feedback_q_inv_d_inside_submodule, toggle_flop
// CLUE: a toggle flop entirely inside a submodule: Q -> INV_X1 -> D. The
// combinational loop closes through the flop, inside one module.

module tog (input ck, input rn, output q);
  wire qi, dn;
  DFFR_X1 ff (.D(dn), .RN(rn), .CK(ck), .Q(qi));
  INV_X1 i0 (.A(qi), .ZN(dn));
  assign q = qi;
endmodule

module top (input ck, input rn, output q);
  tog u (.ck(ck), .rn(rn), .q(q));
endmodule
