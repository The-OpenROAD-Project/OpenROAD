// TARGETS: flat_path_collision, escaped_net, depth_4
// CLUE: depth-4 form of the flat-net path collision -- the top escaped net
// \a/b/c/n owns the name the reader synthesizes for net n of a->b->c
// (dbReadVerilog.cc:732 names every flat net with pathName()).  Existing cases
// stop at depth 3, so a fix that special-cases one join level would pass those.
module m4c (input a, output z);
  wire n;
  INV_X1 g1 (.A(a), .ZN(n));
  INV_X1 g2 (.A(n), .ZN(z));
endmodule

module m4b (input a, output z);
  m4c c (.a(a), .z(z));
endmodule

module m4a (input a, output z);
  m4b b (.a(a), .z(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  wire \a/b/c/n ;
  m4a a (.a(i1), .z(o1));
  INV_X1 g3 (.A(i2), .ZN(\a/b/c/n ));
  INV_X1 g4 (.A(\a/b/c/n ), .ZN(o2));
endmodule
