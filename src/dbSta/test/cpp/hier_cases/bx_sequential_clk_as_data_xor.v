// TOP: top
// TECH: nangate45
// TARGETS: clock_net_also_xor_data_input, clock_as_data
// CLUE: the real clock net feeds both a flop's CK inside a submodule and a
// XOR2_X1 data input at the top -- one net used in two distinct roles.

module ffmod (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module top (input d, input ck, output q, output z);
  wire qi;
  ffmod u (.d(d), .ck(ck), .q(qi));
  XOR2_X1 g (.A(ck), .B(qi), .Z(z));
  assign q = qi;
endmodule
