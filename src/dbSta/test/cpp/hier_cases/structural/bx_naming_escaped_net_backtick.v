// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_backtick, depth_1
// CLUE: net named \`def ; a backtick inside an identifier looks like a
// compiler directive if the escape is lost.
module top (input a, output z);
  wire \`def ;
  INV_X1 u1 (.A(a), .ZN(\`def ));
  INV_X1 u2 (.A(\`def ), .ZN(z));
endmodule
