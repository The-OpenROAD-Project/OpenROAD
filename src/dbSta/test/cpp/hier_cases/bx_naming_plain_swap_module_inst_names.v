// TOP: top
// TECH: nangate45
// TARGETS: name_interplay, instance_module_swap, depth_1
// CLUE: Modules x and y (distinct functions) instantiated as 'x y(...)' and 'y x(...)' — instance names swap the module names; a binder confusing the namespaces changes logic.
module x (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module y (i, o);
  input i;
  output o;
  BUF_X1 g (.A(i), .Z(o));
endmodule
module top (a, z);
  input a;
  output z;
  wire w1, w2;
  x y (.i(a), .o(w1));
  y x (.i(w1), .o(w2));
  INV_X1 u1 (.A(w2), .ZN(z));
endmodule
