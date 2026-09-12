// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, flat_net_collision, depth_3
// CLUE: top-level NET \a/b/n vs 2-level hierarchy a->b with internal net n;
// deeper variant of the net-name collision.
module m_bn (input a, output z);
  wire n;
  INV_X1 g1 (.A(a), .ZN(n));
  INV_X1 g2 (.A(n), .ZN(z));
endmodule

module m_an (input a, output z);
  m_bn b (.a(a), .z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  wire \a/b/n ;
  m_an a (.a(in1), .z(o1));
  INV_X1 g3 (.A(in2), .ZN(\a/b/n ));
  INV_X1 g4 (.A(\a/b/n ), .ZN(o2));
endmodule
