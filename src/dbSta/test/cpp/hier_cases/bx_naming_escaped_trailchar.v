// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, trailing_char_pair, depth_1
// CLUE: two nets \q*  and \q&  differing only in the final char; a writer
// that truncates or canonicalizes punctuation merges them.
module top (input a, output z, output y);
  wire \q* ;
  wire \q& ;
  INV_X1 g1 (.A(a), .ZN(\q* ));
  BUF_X1 g2 (.A(a), .Z(\q& ));
  BUF_X1 g3 (.A(\q* ), .Z(z));
  INV_X1 g4 (.A(\q& ), .ZN(y));
endmodule
