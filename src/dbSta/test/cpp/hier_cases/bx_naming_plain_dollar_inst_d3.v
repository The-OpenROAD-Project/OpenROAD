// TOP: top
// TECH: nangate45
// TARGETS: dollar_name, instance, depth_3
// CLUE: $ in module-instance names at each level; flat path m$1/l$2/c$$3 must survive uniquification/escaping in the flat writer.
module leaf (i, o);
  input i;
  output o;
  INV_X1 c$$3 (.A(i), .ZN(o));
endmodule
module mid (i, o);
  input i;
  output o;
  wire w;
  leaf l$2 (.i(i), .o(w));
  BUF_X1 b$ (.A(w), .Z(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  mid m$1 (.i(a), .o(w));
  INV_X1 u1 (.A(w), .ZN(y));
endmodule
