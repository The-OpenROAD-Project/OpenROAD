// TOP: top
// TECH: nangate45
// TARGETS: escaped_net_vs_port_connected_net
// CLUE: top NET \x/p vs the net bound to output PORT p of submodule x; when
// flattening, port-connected nets may be renamed to x/p and collide.
module sub (input a, output p);
  INV_X1 g1 (.A(a), .ZN(p));
endmodule

module top (input i1, input i2, output o1, output o2);
  wire \x/p ;
  sub x (.a(i1), .p(o1));
  INV_X1 g3 (.A(i2), .ZN(\x/p ));
  INV_X1 g4 (.A(\x/p ), .ZN(o2));
endmodule
