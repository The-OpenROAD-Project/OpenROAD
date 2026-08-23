// TOP: top
// TECH: nangate45
// TARGETS: escaped_lexes_plain, top_module, depth_1
// CLUE: TOP module declared as \top ; link_design is asked for plain
// "top" -- per LRM they are the same identifier.
module \top (input a, output z);
  INV_X1 g1 (.A(a), .ZN(z));
endmodule
