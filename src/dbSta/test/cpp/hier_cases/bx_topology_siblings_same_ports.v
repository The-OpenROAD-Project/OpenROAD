// TOP: top
// TECH: nangate45
// TARGETS: sibling_types_identical_port_names, uniquification
// CLUE: two DIFFERENT sibling module types with identical port names (i,o)
// instantiated side by side in top; port-name identity across types must not
// confuse linking or writing.

module ta (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module tb (input i, output o);
  BUF_X1 g (.A(i), .Z(o));
endmodule

module top (input a, input b, output x, output y);
  ta u1 (.i(a), .o(x));
  tb u2 (.i(b), .o(y));
endmodule
