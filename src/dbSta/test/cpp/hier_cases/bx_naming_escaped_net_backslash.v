// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_backslash, depth_1
// CLUE: net named \a\b  -- escaped id CONTAINING a backslash (legal, any
// printable ASCII). Writers that re-escape naively may double or eat it.
module top (input a, output z);
  wire \a\b ;
  BUF_X1 u1 (.A(a), .Z(\a\b ));
  INV_X1 u2 (.A(\a\b ), .ZN(z));
endmodule
