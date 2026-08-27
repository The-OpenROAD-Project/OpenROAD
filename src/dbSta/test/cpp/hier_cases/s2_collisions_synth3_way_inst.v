// TARGETS: three_way_path_collision, escaped_inst, flat_path_collision, depth_3
// CLUE: THREE distinct leaf instances, three distinct sta names
// (a/b\/c, a\/b/c, a\/b\/c), one emitted Verilog name \a/b/c .  Existing cases
// collide a pair; this proves the merge is not a pairwise accident and that the
// victim count is not bounded -- the flat writer emits three INV_X1 \a/b/c .
module modA3 (input a, output z);
  INV_X1 \b/c  (.A(a), .ZN(z));
endmodule

module modB3 (input a, output z);
  INV_X1 c (.A(a), .ZN(z));
endmodule

module top (input i1, input i2, input i3, output o1, output o2, output o3);
  modA3 a (.a(i1), .z(o1));
  modB3 \a/b  (.a(i2), .z(o2));
  INV_X1 \a/b/c  (.A(i3), .ZN(o3));
endmodule
