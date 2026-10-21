// TOP: top
// TECH: nangate45
// TARGETS: mux_hold_feedback_inside_submodule, load_enable_register
// CLUE: classic load-enable register: MUX2_X1 selects between the flop's own Q
// (hold) and new data, all inside the submodule.

module reg_en (input d, input en, input ck, input rn, output q);
  wire qi, dm;
  MUX2_X1 m (.A(qi), .B(d), .S(en), .Z(dm));
  DFFR_X1 ff (.D(dm), .RN(rn), .CK(ck), .Q(qi));
  assign q = qi;
endmodule

module top (input d, input en, input ck, input rn, output q);
  reg_en u (.d(d), .en(en), .ck(ck), .rn(rn), .q(q));
endmodule
