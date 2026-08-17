// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, char_dot, depth_1
// CLUE: instance named \i.j ; a dot in an instance name mimics a
// hierarchical path element.
module top (input a, output z);
  INV_X1 \i.j (.A(a), .ZN(z));
endmodule
