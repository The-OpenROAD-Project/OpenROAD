// TARGETS: two_synthesized_net_paths_collide, escaped_in_sub, depth_4
// CLUE: no user object owns the colliding name.  Net \c/n inside b inside a
// flattens to a/b/c\/n; net n inside c inside instance \a/b flattens to
// a\/b/c/n.  The two sta names are different strings; staToVerilog drops the
// escape (VerilogNamespace.cc:88-97) so both print \a/b/c/n .  A 3+1 split
// against a 2+2 split -- existing synthesized-pair cases only do 1+2.
module s4b (input a, output z);
  wire \c/n ;
  INV_X1 g1 (.A(a), .ZN(\c/n ));
  INV_X1 g2 (.A(\c/n ), .ZN(z));
endmodule

module s4a (input a, output z);
  s4b b (.a(a), .z(z));
endmodule

module s4c (input a, output z);
  wire n;
  INV_X1 g1 (.A(a), .ZN(n));
  INV_X1 g2 (.A(n), .ZN(z));
endmodule

module s4d (input a, output z);
  s4c c (.a(a), .z(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  s4a a (.a(i1), .z(o1));
  s4d \a/b  (.a(i2), .z(o2));
endmodule
