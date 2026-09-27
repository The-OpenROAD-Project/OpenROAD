// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, slash_path_collision, unconnected_port, depth_2
// CLUE: top net \x/p  plus instance x of a module with output port p left
// unconnected; the flat writer names the dangling driver net x/p --
// collision through a PORT path (new variant).
module subp (input a, output p, output z);
  INV_X1 g1 (.A(a), .ZN(p));
  BUF_X1 g2 (.A(a), .Z(z));
endmodule
module top (input a, output z, output w);
  wire \x/p ;
  subp x (.a(a), .z(w));
  BUF_X1 g1 (.A(a), .Z(\x/p ));
  BUF_X1 g2 (.A(\x/p ), .Z(z));
endmodule
