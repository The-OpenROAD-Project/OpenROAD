// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, char_dot, depth_2
// CLUE: instance \g.h  inside a submodule; flat writer emits path u1/g.h
// which must remain escaped.
module subi (input a, output z);
  INV_X1 \g.h (.A(a), .ZN(z));
endmodule
module top (input a, output z);
  subi u1 (.a(a), .z(z));
endmodule
