// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_dollar_leading, depth_1
// CLUE: net named \$w ; $ is legal in plain ids except as FIRST char, so
// this name needs the escape forever -- normalizers that strip escapes
// from "word-like" names emit illegal $w.
module top (input a, output z);
  wire \$w ;
  INV_X1 u1 (.A(a), .ZN(\$w ));
  INV_X1 u2 (.A(\$w ), .ZN(z));
endmodule
