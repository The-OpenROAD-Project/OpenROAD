// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, boundary_net_naming, flat_net_collision
// CLUE: the boundary net of port p of instance x is named w1 by the parent;
// if the flattener prefers the CHILD-side name it becomes x/p and collides
// with the unrelated top escaped net \x/p .
module subp2 (input a, output p);
  INV_X1 g1 (.A(a), .ZN(p));
endmodule

module top (input i1, input i2, output o1, output o2);
  wire w1;
  wire \x/p ;
  subp2 x (.a(i1), .p(w1));
  INV_X1 g2 (.A(w1), .ZN(o1));
  INV_X1 g3 (.A(i2), .ZN(\x/p ));
  INV_X1 g4 (.A(\x/p ), .ZN(o2));
endmodule
