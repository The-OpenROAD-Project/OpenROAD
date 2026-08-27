// TARGETS: escaped_net, char_uncovered_punct, depth_1
// CLUE: '?', '~', '<', '>' and '|' appear in ZERO escaped identifiers in the
// existing corpus. staToVerilog2 (VerilogNamespace.cc:120-158) decides "needs
// escaping" purely from isAlnumUnderscore, so each of these must set the escape
// flag; four nets differing only in that one middle character also prove no
// canonicalisation merges them.
module top (a, y0, y1, y2, y3);
  input a;
  output y0;
  output y1;
  output y2;
  output y3;
  wire \a?b ;
  wire \a~b ;
  wire \a<b ;
  wire \a|b ;
  INV_X1 g0 (.A(a), .ZN(\a?b ));
  BUF_X1 g1 (.A(a), .Z(\a~b ));
  INV_X1 g2 (.A(a), .ZN(\a<b ));
  BUF_X1 g3 (.A(a), .Z(\a|b ));
  BUF_X1 h0 (.A(\a?b ), .Z(y0));
  INV_X1 h1 (.A(\a~b ), .ZN(y1));
  BUF_X1 h2 (.A(\a<b ), .Z(y2));
  INV_X1 h3 (.A(\a|b ), .ZN(y3));
endmodule
