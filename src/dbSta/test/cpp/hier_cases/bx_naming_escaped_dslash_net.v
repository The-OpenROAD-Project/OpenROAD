// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, double_slash, depth_1
// CLUE: net named \a//b ; emitted unescaped the tail becomes a line
// comment, silently truncating the statement.
module top (input a, output z);
  wire \a//b ;
  INV_X1 u1 (.A(a), .ZN(\a//b ));
  INV_X1 u2 (.A(\a//b ), .ZN(z));
endmodule
