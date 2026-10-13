// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, flat_path_collision, mixed_level
// CLUE: escaped instance \x/y/z in top vs hierarchy x whose module contains
// an ESCAPED instance \y/z ; both flatten to path x/y/z.
module mx (input a, output z);
  INV_X1 \y/z  (.A(a), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  mx x (.a(in1), .z(o1));
  INV_X1 \x/y/z  (.A(in2), .ZN(o2));
endmodule
