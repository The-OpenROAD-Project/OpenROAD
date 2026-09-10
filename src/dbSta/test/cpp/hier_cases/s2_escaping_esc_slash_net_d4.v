// TARGETS: escaped_net, naming_slash, depth_4
// CLUE: the divider-vs-escape ambiguity at maximum depth. Net `\a/b ` has sta
// name "a\/b", so its flat dbNet name is "u1/u2/u3/a\/b" -- FOUR '/' of which
// only the first three are dividers. dbNetwork::name(Net) tests
// name.find_last_of('/') (dbNetwork.cc:2605) with no escape awareness at all,
// and then erases an unanchored prefix (dbNetwork.cc:2628-2630). Corpus covers
// this shape only to depth 3.
module leaf4 (input a, output z);
  wire \a/b ;
  INV_X1 g1 (.A(a), .ZN(\a/b ));
  BUF_X1 g2 (.A(\a/b ), .Z(z));
endmodule
module mid4 (input a, output z);
  leaf4 u3 (.a(a), .z(z));
endmodule
module mid3 (input a, output z);
  mid4 u2 (.a(a), .z(z));
endmodule
module top (input a, output z);
  mid3 u1 (.a(a), .z(z));
endmodule
