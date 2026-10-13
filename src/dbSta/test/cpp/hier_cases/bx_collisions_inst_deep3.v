// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, flat_path_collision, depth_3
// CLUE: escaped instance \a/b/c in top vs 3-level hierarchy a->b->c
// (leaf instance c at the bottom); deeper variant of known finding 3.
module m_c (input a, output z);
  INV_X1 c (.A(a), .ZN(z));
endmodule

module m_b (input a, output z);
  m_c b (.a(a), .z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  m_b a (.a(in1), .z(o1));
  INV_X1 \a/b/c  (.A(in2), .ZN(o2));
endmodule
