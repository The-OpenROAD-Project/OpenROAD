// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_equals, depth_1
// CLUE: net named \a=b ; equals unescaped turns a wire reference into an
// assignment-looking token stream.
module top (input a, input b, output z);
  wire \a=b ;
  XNOR2_X1 u1 (.A(a), .B(b), .ZN(\a=b ));
  BUF_X1 u2 (.A(\a=b ), .Z(z));
endmodule
