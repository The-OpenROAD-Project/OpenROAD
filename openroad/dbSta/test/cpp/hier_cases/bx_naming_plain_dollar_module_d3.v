// TOP: top
// TECH: nangate45
// TARGETS: dollar_name, module_name, depth_3
// CLUE: $ in every module name of a depth-3 chain (m$a -> m$b).
module m$b (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module m$a (i, o);
  input i;
  output o;
  wire w;
  m$b u (.i(i), .o(w));
  BUF_X1 b (.A(w), .Z(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  m$a u (.i(a), .o(w));
  INV_X1 v (.A(w), .ZN(y));
endmodule
