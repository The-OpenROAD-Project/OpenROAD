// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_plus, depth_1
// CLUE: net named \a+b ; if writer drops the escape or trailing space the
// name parses as expression a+b.
module top (input a, input b, output z);
  wire \a+b ;
  AND2_X1 u1 (.A1(a), .A2(b), .ZN(\a+b ));
  BUF_X1 u2 (.A(\a+b ), .Z(z));
endmodule
