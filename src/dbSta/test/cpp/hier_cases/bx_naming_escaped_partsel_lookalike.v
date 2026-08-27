// TOP: top
// TECH: nangate45
// TARGETS: escaped_partsel_lookalike, depth_1
// CLUE: scalar net named \a[1:0]  while a scalar port a exists; unescaped
// it reads as an illegal part-select of scalar a.
module top (input a, output z);
  wire \a[1:0] ;
  BUF_X1 g1 (.A(a), .Z(\a[1:0] ));
  BUF_X1 g2 (.A(\a[1:0] ), .Z(z));
endmodule
