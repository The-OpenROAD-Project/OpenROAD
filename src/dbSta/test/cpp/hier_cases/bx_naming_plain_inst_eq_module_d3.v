// TOP: top
// TECH: nangate45
// TARGETS: name_interplay, instance_eq_module, depth_3
// CLUE: Every level instantiates its child with the child's own name: m1 m1(...), m2 m2(...); flat path m1/m2/g.
module m2 (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module m1 (i, o);
  input i;
  output o;
  wire w;
  m2 m2 (.i(i), .o(w));
  BUF_X1 b (.A(w), .Z(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  m1 m1 (.i(a), .o(w));
  INV_X1 u (.A(w), .ZN(y));
endmodule
