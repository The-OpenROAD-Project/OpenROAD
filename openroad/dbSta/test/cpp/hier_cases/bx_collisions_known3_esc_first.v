// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, flat_path_collision, decl_order
// CLUE: known-finding-3 shape but with the escaped instance declared BEFORE
// the hierarchy instance; probes order sensitivity of the collision.
module subx (input a, output z);
  INV_X1 y (.A(a), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output o2);
  INV_X1 \x/y  (.A(in2), .ZN(o2));
  subx x (.a(in1), .z(o1));
endmodule
