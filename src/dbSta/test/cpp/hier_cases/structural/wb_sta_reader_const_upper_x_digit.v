// TOP: top
// TECH: nangate45
// TARGETS: constant, x_digit, lexer_narrowing
// CLUE: the CONSTANT lexer rules accept only lower case x/z digits ([01_xz], [0-9a-fA-F_xz]),
// CLUE: so 1'bX (legal Verilog, same as 1'bx which IS accepted) never matches and the
// CLUE: bare quote reaches the catch-all "." rule -> STA-0171 syntax error.
module top (a, y);
  input a;
  output y;
  NAND2_X1 g (.A1(a), .A2(1'bX), .ZN(y));
endmodule
