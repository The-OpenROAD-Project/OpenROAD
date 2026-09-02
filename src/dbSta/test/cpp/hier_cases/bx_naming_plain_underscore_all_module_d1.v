// TOP: top
// TECH: nangate45
// TARGETS: all_underscore, module_name, depth_1
// CLUE: All-underscore module name (____) with all-underscore instance name (__).
module ____ (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  ____ __ (.i(a), .o(w));
  INV_X1 u1 (.A(w), .ZN(y));
endmodule
