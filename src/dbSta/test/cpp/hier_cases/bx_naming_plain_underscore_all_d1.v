// TOP: top
// TECH: nangate45
// TARGETS: all_underscore, net, instance, depth_1
// CLUE: All-underscore identifiers: nets _ and ____, instance ___.
module top (a, y);
  input a;
  output y;
  wire _;
  wire ____;
  INV_X1 ___ (.A(a), .ZN(_));
  BUF_X1 __x (.A(_), .Z(____));
  INV_X1 u3 (.A(____), .ZN(y));
endmodule
