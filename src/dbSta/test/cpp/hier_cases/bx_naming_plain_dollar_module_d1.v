// TOP: top
// TECH: nangate45
// TARGETS: dollar_name, module_name, depth_1
// CLUE: $ in a submodule definition name (sub$1) instantiated by top.
module sub$1 (i, o);
  input i;
  output o;
  wire n;
  INV_X1 g1 (.A(i), .ZN(n));
  INV_X1 g2 (.A(n), .ZN(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  sub$1 u1 (.i(a), .o(w));
  INV_X1 u2 (.A(w), .ZN(y));
endmodule
