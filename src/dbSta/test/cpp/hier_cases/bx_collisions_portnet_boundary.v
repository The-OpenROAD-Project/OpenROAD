// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, boundary_net_naming
// CLUE: top net \x/p is CONNECTED to port p of instance x; the flattened
// boundary net's child-side name x/p equals the parent-side escaped name --
// same signal, so the writer must not emit two nets or drop one.
module subp (input a, output p);
  INV_X1 g1 (.A(a), .ZN(p));
endmodule

module top (input in1, output o1);
  wire \x/p ;
  subp x (.a(in1), .p(\x/p ));
  INV_X1 g2 (.A(\x/p ), .ZN(o1));
endmodule
