// TOP: top
// TECH: nangate45
// TARGETS: leaf_cell_positional, coexisting_named_instance, offset_probe
// CLUE: two NAND2_X1 in the same module, one named-connected and one
// positional, both feeding dead loads. Isolates the mis-binding to the
// positional instance only.
module top (input p1, input p2, output y);
  wire za;
  wire zb;
  wire dza;
  wire dzb;
  INV_X1 keep (.A(p1), .ZN(y));
  NAND2_X1 gnamed (.A1(p1), .A2(p2), .ZN(za));
  NAND2_X1 gpos (p1, p2, zb);
  INV_X1 sa (.A(za), .ZN(dza));
  INV_X1 sb (.A(zb), .ZN(dzb));
endmodule
