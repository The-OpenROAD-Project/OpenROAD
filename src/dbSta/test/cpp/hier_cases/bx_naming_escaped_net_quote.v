// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_quote, depth_1
// CLUE: net named \a"b ; a double quote inside an identifier can break
// writers that quote names in strings or reports.
module top (input a, output z);
  wire \a"b ;
  INV_X1 u1 (.A(a), .ZN(\a"b ));
  INV_X1 u2 (.A(\a"b ), .ZN(z));
endmodule
