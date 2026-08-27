// TOP: top
// TECH: nangate45
// TARGETS: top_input_unused, escaped_name
// CLUE: escaped-name top input \nc! is never referenced. Dangling + escaped:
//       writer must keep both the port and its escaping.
module top (x, \nc! , y);
  input x;
  input \nc! ;
  output y;
  INV_X1 u1 (.A(x), .ZN(y));
endmodule
