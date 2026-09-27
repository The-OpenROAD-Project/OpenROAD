// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, inside_submodule, offset_probe, dead_cone
// CLUE: same positional liberty-cell instance but one hierarchy level down.
// Checks the mis-binding is not a top-module-only artifact.
module sub (input a, input b, output y);
  wire z;
  wire dz;
  INV_X1 g1 (.A(a), .ZN(y));
  NAND2_X1 g2 (a, b, z);
  INV_X1 sink (.A(z), .ZN(dz));
endmodule
module top (input in1, input in2, output out1);
  sub u1 (.a(in1), .b(in2), .y(out1));
endmodule
