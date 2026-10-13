// TOP: top
// TECH: nangate45
// TARGETS: cellname_shadow, module_name, depth_1
// CLUE: User module named AND2_X1 whose function is NOR (differs from the liberty cell). If the linker binds the liberty cell instead of the module, logic changes and LEC catches it.
module AND2_X1 (A1, A2, ZN);
  input A1, A2;
  output ZN;
  NOR2_X1 g1 (.A1(A1), .A2(A2), .ZN(ZN));
endmodule
module top (a, b, y);
  input a, b;
  output y;
  wire w;
  AND2_X1 u1 (.A1(a), .A2(b), .ZN(w));
  INV_X1 u2 (.A(w), .ZN(y));
endmodule
