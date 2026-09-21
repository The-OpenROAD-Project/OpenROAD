// TOP: top
// TECH: nangate45
// TARGETS: escaped_module, char_plus, depth_3
// CLUE: escaped module \m+2 instantiated from inside a depth-2 submodule
module top (a, z);
  input a;
  output z;
  sub s1 (.i(a), .o(z));
endmodule
module sub (i, o);
  input i;
  output o;
  \m+2 u1 (.i(i), .o(o));
endmodule
module \m+2 (i, o);
  input i;
  output o;
  INV_X1 g1 (.A(i), .ZN(o));
endmodule
