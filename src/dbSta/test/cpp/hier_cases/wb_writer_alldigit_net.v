// TOP: top
// TECH: nangate45
// TARGETS: needs_escape, numeric_identifier, net_name
// CLUE: an escaped net name consisting only of digits ("\32 ") is all-alnum, so the
// CLUE: writer emits "wire 32;" -- the name is re-lexed as a decimal number.
module top (a, y);
  input a;
  output y;
  wire \32 ;
  INV_X1 g1 (.A(a), .ZN(\32 ));
  BUF_X1 g2 (.A(\32 ), .Z(y));
endmodule
