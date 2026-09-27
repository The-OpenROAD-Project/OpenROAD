// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, keyword_wire, depth_1
// CLUE: net named \wire ; emitting it unescaped produces "wire wire;"-like
// text that may still parse but denote the wrong thing.
module top (input a, output z);
  wire \wire ;
  BUF_X1 u1 (.A(a), .Z(\wire ));
  BUF_X1 u2 (.A(\wire ), .Z(z));
endmodule
