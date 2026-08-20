// TOP: top
// TECH: nangate45
// TARGETS: case_sensitivity, module_name, depth_1
// CLUE: Modules sub, SUB, Sub as three distinct definitions (different functions and port counts) all instantiated by top.
module sub (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module SUB (i, o);
  input i;
  output o;
  BUF_X1 g (.A(i), .Z(o));
endmodule
module Sub (i, j, o);
  input i, j;
  output o;
  AND2_X1 g (.A1(i), .A2(j), .ZN(o));
endmodule
module top (a, b, y);
  input a, b;
  output y;
  wire w1, w2, w3;
  sub u1 (.i(a), .o(w1));
  SUB u2 (.i(b), .o(w2));
  Sub u3 (.i(w1), .j(w2), .o(w3));
  INV_X1 u4 (.A(w3), .ZN(y));
endmodule
