// TOP: top
// TECH: nangate45
// TARGETS: name_interplay, net_eq_module, depth_1
// CLUE: Wire named 'sub' in top while module sub is instantiated in the same scope (different namespaces, both legal).
module sub (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire sub;
  sub u1 (.i(a), .o(sub));
  INV_X1 u2 (.A(sub), .ZN(y));
endmodule
