// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, char_plus, char_minus, depth_3
// CLUE: escaped port names \i+n  / \o-t  passed through TWO hierarchy
// levels; every boundary re-emission must keep the escapes.
module leafp (input \i+n , output \o-t );
  INV_X1 g1 (.A(\i+n ), .ZN(\o-t ));
endmodule
module midp (input \i+n , output \o-t );
  leafp u2 (.\i+n (\i+n ), .\o-t (\o-t ));
endmodule
module top (input a, output z);
  midp u1 (.\i+n (a), .\o-t (z));
endmodule
