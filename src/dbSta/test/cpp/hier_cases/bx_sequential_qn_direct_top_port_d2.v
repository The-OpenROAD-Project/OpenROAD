// TOP: top
// TECH: nangate45
// TARGETS: qn_through_two_feedthrough_levels, no_logic_on_path
// CLUE: QN goes straight from the leaf flop to a top output through two
// levels of pure port feedthrough -- nothing but boundaries on the path.

module leaf (input d, input ck, output qn);
  DFF_X1 ff (.D(d), .CK(ck), .QN(qn));
endmodule

module mid (input d, input ck, output qn);
  leaf c (.d(d), .ck(ck), .qn(qn));
endmodule

module top (input d, input ck, output zn);
  mid u (.d(d), .ck(ck), .qn(zn));
endmodule
