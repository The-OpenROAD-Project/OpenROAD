// TOP: top
// TECH: nangate45
// TARGETS: uniquification_collides_with_liberty_cell_name
// CLUE: module named INV instantiated twice, second instance named X1; the
// hier uniquification scheme <module>_<inst> would produce module name
// INV_X1 which is a Nangate45 LIBERTY CELL name — any downstream reader
// resolves it to the cell (pins A/ZN) instead of the module (ports i/o).

module INV (input i, output o);
  NAND2_X1 g (.A1(i), .A2(i), .ZN(o));
endmodule

module top (input a, input b, output x, output y);
  INV X0 (.i(a), .o(x));
  INV X1 (.i(b), .o(y));
endmodule
