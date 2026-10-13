// TOP: top
// TECH: nangate45
// TARGETS: leading_underscore, net, instance, depth_1
// CLUE: Leading-underscore net and instance names (_n1, __n2, _u1, __u2).
module top (a, y);
  input a;
  output y;
  wire _n1, __n2;
  INV_X1 _u1 (.A(a), .ZN(_n1));
  BUF_X1 __u2 (.A(_n1), .Z(__n2));
  INV_X1 u3 (.A(__n2), .ZN(y));
endmodule
