// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, flat_path_prefix, mid_path
// CLUE: escaped leaf instance \a/b in top vs hierarchy a containing module
// instance b containing logic; flat names a/b and a/b/u1 are prefix-related
// but distinct -- probes prefix handling, not an exact-name collision.
module m_leaf (input a, output z);
  INV_X1 u1 (.A(a), .ZN(z));
endmodule

module m_a (input a, output z);
  m_leaf b (.a(a), .z(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  m_a a (.a(in1), .z(o1));
  INV_X1 \a/b  (.A(in2), .ZN(o2));
endmodule
