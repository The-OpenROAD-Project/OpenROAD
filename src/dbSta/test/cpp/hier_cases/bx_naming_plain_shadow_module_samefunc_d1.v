// TOP: top
// TECH: nangate45
// TARGETS: cellname_shadow, module_name, depth_1
// CLUE: User module named AND2_X1 (function-equivalent to the liberty cell, built from NAND2+INV). Linker must bind the module; LEC passes either way, so check structure of the hier output.
module AND2_X1 (A1, A2, ZN);
  input A1, A2;
  output ZN;
  wire n;
  NAND2_X1 g1 (.A1(A1), .A2(A2), .ZN(n));
  INV_X1 g2 (.A(n), .ZN(ZN));
endmodule
module top (a, b, y);
  input a, b;
  output y;
  wire w;
  AND2_X1 u1 (.A1(a), .A2(b), .ZN(w));
  INV_X1 u2 (.A(w), .ZN(y));
endmodule
