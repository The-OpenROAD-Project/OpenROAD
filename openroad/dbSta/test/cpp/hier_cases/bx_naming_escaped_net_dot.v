// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_dot, depth_1
// CLUE: net named \a.b ; dot is the classic hierarchy separator, writers
// that split on '.' will mangle it.
module top (input a, input b, output z);
  wire \a.b ;
  NAND2_X1 u1 (.A1(a), .A2(b), .ZN(\a.b ));
  INV_X1 u2 (.A(\a.b ), .ZN(z));
endmodule
