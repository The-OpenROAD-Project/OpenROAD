// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, clone_name_equals_liberty_cell
// CLUE: module INV instantiated as A0 and X1; the <module>_<inst> clone name
// for X1 is INV_X1 -- a real Nangate45 cell that the clone itself
// instantiates. A module definition shadowing a liberty cell name.
module INV (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  INV A0 (.a(in1), .z(o1));
  INV X1 (.a(in2), .z(o2));
endmodule
