// TOP: top
// TECH: nangate45
// TARGETS: cellname_shadow, net, depth_1
// CLUE: Nets named BUF_X1 and DFF_X1 (liberty cell names) in a scope that also instantiates real cells.
module top (a, y);
  input a;
  output y;
  wire BUF_X1;
  wire DFF_X1;
  INV_X1 u1 (.A(a), .ZN(BUF_X1));
  BUF_X1 u2 (.A(BUF_X1), .Z(DFF_X1));
  INV_X1 u3 (.A(DFF_X1), .ZN(y));
endmodule
