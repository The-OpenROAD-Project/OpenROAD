// TOP: top
// TECH: nangate45
// TARGETS: module_shadows_liberty_cell, second_generation_roundtrip
// CLUE: this is the verbatim hier output of bx_topology_uniq_cell_collision;
// it defines module INV_X1 shadowing the Nangate45 cell INV_X1. Does
// OpenROAD re-reading it resolve INV_X1 to the module or the cell?
module top (a,
    b,
    x,
    y);
 input a;
 input b;
 output x;
 output y;


 INV X0 (.i(a),
    .o(x));
 INV_X1 X1 (.i(b),
    .o(y));
endmodule
module INV (i,
    o);
 input i;
 output o;


 NAND2_X1 g (.A1(i),
    .A2(i),
    .ZN(o));
endmodule
module INV_X1 (i,
    o);
 input i;
 output o;


 NAND2_X1 g (.A1(i),
    .A2(i),
    .ZN(o));
endmodule
