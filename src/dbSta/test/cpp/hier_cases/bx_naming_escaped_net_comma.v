// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_comma, depth_1
// CLUE: net named \a,b ; comma unescaped splits port connection lists.
module top (input a, input b, output z);
  wire \a,b ;
  XOR2_X1 u1 (.A(a), .B(b), .Z(\a,b ));
  BUF_X1 u2 (.A(\a,b ), .Z(z));
endmodule
