// TOP: top
// TECH: nangate45
// TARGETS: two_synthesized_net_paths_collide, escaped_in_sub
// CLUE: internal net \b/n of instance a and internal net n of instance \a/b
// both flatten to net a/b/n -- synthesized NET collision, may merge cones.
module modAN (input a, output z);
  wire \b/n ;
  INV_X1 g1 (.A(a), .ZN(\b/n ));
  INV_X1 g2 (.A(\b/n ), .ZN(z));
endmodule

module modBN (input a, output z);
  wire n;
  INV_X1 g1 (.A(a), .ZN(n));
  INV_X1 g2 (.A(n), .ZN(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  modAN a (.a(i1), .z(o1));
  modBN \a/b  (.a(i2), .z(o2));
endmodule
