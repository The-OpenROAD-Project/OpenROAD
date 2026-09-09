// TOP: top
// TECH: nangate45
// TARGETS: two_synthesized_paths_collide, escaped_in_sub, escaped_top_inst
// CLUE: neither name exists literally: instance a holds cell \b/c and instance
// \a/b holds cell c; BOTH flatten to instance path a/b/c -- synthesized collision.
module modA (input a, output z);
  INV_X1 \b/c  (.A(a), .ZN(z));
endmodule

module modB (input a, output z);
  INV_X1 c (.A(a), .ZN(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  modA a (.a(i1), .z(o1));
  modB \a/b  (.a(i2), .z(o2));
endmodule
