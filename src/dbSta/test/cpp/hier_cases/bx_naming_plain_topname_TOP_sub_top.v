// TOP: TOP
// TECH: nangate45
// TARGETS: name_interplay, case_sensitivity, top_name
// CLUE: Top module named TOP instantiating a submodule named top; link_design TOP must not grab the submodule.
module top (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module TOP (a, y);
  input a;
  output y;
  wire w;
  top u1 (.i(a), .o(w));
  INV_X1 u2 (.A(w), .ZN(y));
endmodule
