// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_paren, depth_1
// CLUE: net named \(a) ; parens unescaped break port-connection syntax.
module top (input a, output z);
  wire \(a) ;
  INV_X1 u1 (.A(a), .ZN(\(a) ));
  INV_X1 u2 (.A(\(a) ), .ZN(z));
endmodule
