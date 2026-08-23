// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, flat_path_prefix, mid_path
// CLUE: top escaped net \a/b is a strict PREFIX of flattened net a/b/n from
// hierarchy a->b; distinct names, probes prefix handling in the flat writer.
module m_leafn (input a, output z);
  wire n;
  INV_X1 g1 (.A(a), .ZN(n));
  INV_X1 g2 (.A(n), .ZN(z));
endmodule

module m_an2 (input a, output z);
  m_leafn b (.a(a), .z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  wire \a/b ;
  m_an2 a (.a(in1), .z(o1));
  INV_X1 g3 (.A(in2), .ZN(\a/b ));
  INV_X1 g4 (.A(\a/b ), .ZN(o2));
endmodule
