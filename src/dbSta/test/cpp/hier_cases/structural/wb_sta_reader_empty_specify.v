// TOP: top
// TECH: nangate45
// TARGETS: specify_block, grammar_gap
// CLUE: specify_stmts (VerilogParse.yy:252) requires at least one SPECPARAM, so the
// CLUE: legal empty specify/endspecify pair is a syntax error.
module top (a, y);
  input a;
  output y;
  specify
  endspecify
  INV_X1 g (.A(a), .ZN(y));
endmodule
