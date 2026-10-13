// TOP: top
// TECH: nangate45
// TARGETS: name_interplay, case_sensitivity, top_like_module
// CLUE: Submodule named Top while the top module is top (case-only difference in the definitions namespace).
module Top (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  Top u1 (.i(a), .o(w));
  INV_X1 u2 (.A(w), .ZN(y));
endmodule
