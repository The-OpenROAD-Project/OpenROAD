// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_semicolon, depth_1
// CLUE: net named \a;b ; semicolon unescaped terminates statements early.
module top (input a, input b, output z);
  wire \a;b ;
  NOR2_X1 u1 (.A1(a), .A2(b), .ZN(\a;b ));
  INV_X1 u2 (.A(\a;b ), .ZN(z));
endmodule
