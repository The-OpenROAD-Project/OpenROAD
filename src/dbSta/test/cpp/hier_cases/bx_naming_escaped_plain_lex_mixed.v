// TOP: top
// TECH: nangate45
// TARGETS: escaped_plain_equiv, mixed_spelling, depth_1
// CLUE: net declared as \abc but referenced as plain abc -- LRM says same identifier; reader must unify
module top (a, z);
  input a;
  output z;
  wire \abc ;
  BUF_X1 u1 (.A(a), .Z(\abc ));
  INV_X1 u2 (.A(abc), .ZN(z));
endmodule
