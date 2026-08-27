// TOP: top
// TECH: nangate45
// TARGETS: digits_in_name, module_name, depth_3
// CLUE: Digit-heavy module names (m1a2, k9z9) in a depth-3 chain.
module k9z9 (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module m1a2 (i, o);
  input i;
  output o;
  wire w;
  k9z9 u (.i(i), .o(w));
  BUF_X1 b (.A(w), .Z(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  m1a2 u (.i(a), .o(w));
  INV_X1 v (.A(w), .ZN(y));
endmodule
