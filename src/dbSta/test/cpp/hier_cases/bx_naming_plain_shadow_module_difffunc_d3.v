// TOP: top
// TECH: nangate45
// TARGETS: cellname_shadow, module_name, depth_3
// CLUE: User module named NAND2_X1 (function = OR, differs from cell) instantiated by the leaf of a depth-3 chain.
module NAND2_X1 (A1, A2, ZN);
  input A1, A2;
  output ZN;
  OR2_X1 g1 (.A1(A1), .A2(A2), .ZN(ZN));
endmodule
module leaf (i, j, o);
  input i, j;
  output o;
  NAND2_X1 u (.A1(i), .A2(j), .ZN(o));
endmodule
module mid (i, j, o);
  input i, j;
  output o;
  wire w;
  leaf l (.i(i), .j(j), .o(w));
  BUF_X1 b (.A(w), .Z(o));
endmodule
module top (a, b, y);
  input a, b;
  output y;
  wire w;
  mid m (.i(a), .j(b), .o(w));
  INV_X1 u (.A(w), .ZN(y));
endmodule
