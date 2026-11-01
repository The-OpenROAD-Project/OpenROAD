// TOP: top
// TECH: nangate45
// TARGETS: attribute, continuous_assign, grammar_gap
// CLUE: VerilogParse.yy gives attr_instance_seq to module/declaration/port_dcl/instance
// CLUE: but continuous_assign (line 359, reached through stmt_seq) has none, so a legal
// CLUE: attribute_instance in front of an assign is a syntax error.
module top (a, y);
  input a;
  output y;
  wire n;
  INV_X1 g (.A(a), .ZN(n));
  (* keep = "true" *) assign y = n;
endmodule
