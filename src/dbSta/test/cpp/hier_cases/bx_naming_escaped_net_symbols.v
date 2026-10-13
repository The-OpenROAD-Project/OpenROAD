// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_symbols, depth_1
// CLUE: net named \!@#$%^&* ; dense punctuation soup, every char must
// survive the round trip byte-for-byte (or as the same identifier).
module top (input a, output z);
  wire \!@#$%^&* ;
  INV_X1 u1 (.A(a), .ZN(\!@#$%^&* ));
  INV_X1 u2 (.A(\!@#$%^&* ), .ZN(z));
endmodule
