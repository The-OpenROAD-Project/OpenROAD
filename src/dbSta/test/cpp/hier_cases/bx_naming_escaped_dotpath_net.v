// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, dot_path_collision, depth_2
// CLUE: top net \u1.u2  while real hierarchy instance u1 contains instance
// u2; tools that flatten with '.' separator collide the two names.
module subm (input a, output z);
  INV_X1 u2 (.A(a), .ZN(z));
endmodule
module top (input a, output z, output y);
  wire \u1.u2 ;
  subm u1 (.a(a), .z(y));
  BUF_X1 g1 (.A(a), .Z(\u1.u2 ));
  BUF_X1 g2 (.A(\u1.u2 ), .Z(z));
endmodule
