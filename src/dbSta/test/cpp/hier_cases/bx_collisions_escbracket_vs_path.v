// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, flat_path_collision, bracket_chars
// CLUE: escaped instance \x/y[0] in top vs hierarchy x containing escaped
// instance \y[0] ; both flatten to x/y[0] -- collision with bus-like chars.
module subbr (input a, output z);
  INV_X1 \y[0]  (.A(a), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  subbr x (.a(in1), .z(o1));
  INV_X1 \x/y[0]  (.A(in2), .ZN(o2));
endmodule
