// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, char_plus, char_minus, depth_2
// CLUE: submodule ports named \p+q  and \r-s  connected by name from top;
// hier writer must re-emit .\p+q ( ) with the escape and trailing space.
module subq (input \p+q , output \r-s );
  INV_X1 g1 (.A(\p+q ), .ZN(\r-s ));
endmodule
module top (input a, output z);
  subq u1 (.\p+q (a), .\r-s (z));
endmodule
