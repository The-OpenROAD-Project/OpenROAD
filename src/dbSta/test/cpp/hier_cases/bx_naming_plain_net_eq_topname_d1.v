// TOP: top
// TECH: nangate45
// TARGETS: name_interplay, net_eq_top, depth_1
// CLUE: Wire named 'top' inside module top.
module top (a, y);
  input a;
  output y;
  wire top;
  INV_X1 u1 (.A(a), .ZN(top));
  INV_X1 u2 (.A(top), .ZN(y));
endmodule
