// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, flat_path_collision, depth_2
// CLUE: exact known-finding-3 shape: escaped instance \x/y in top collides
// with the flat path of instance y inside hierarchy instance x.
module subx (input a, output z);
  INV_X1 y (.A(a), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  subx x (.a(in1), .z(o1));
  INV_X1 \x/y  (.A(in2), .ZN(o2));
endmodule
