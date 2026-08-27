// TARGETS: flat_path_collision, escaped_inst, depth_4
// CLUE: depth-4 form of the flat-instance path collision.  The escaped leaf
// instance \a/b/c/g in top owns the name makeChildInsts synthesizes for gate g
// of a->b->c (dbReadVerilog.cc:538, pathName()).  Instance namespace variant of
// the depth-4 net case, and one level deeper than bx_collisions_inst_deep3.
module n4c (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module n4b (input a, output z);
  n4c c (.a(a), .z(z));
endmodule

module n4a (input a, output z);
  n4b b (.a(a), .z(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  n4a a (.a(i1), .z(o1));
  INV_X1 \a/b/c/g  (.A(i2), .ZN(o2));
endmodule
