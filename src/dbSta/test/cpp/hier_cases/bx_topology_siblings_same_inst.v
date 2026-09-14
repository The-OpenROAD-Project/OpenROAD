// TOP: top
// TECH: nangate45
// TARGETS: identical_instance_names_different_parents, shared_leaf, uniquification
// CLUE: two different parent types each contain an instance named u1 of the
// SAME leaf type (leaf gate also named g in both); flat path must synthesize
// distinct full names for ua/u1/g and ub/u1/g.

module lfx (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module pa2 (input i, output o);
  lfx u1 (.i(i), .o(o));
endmodule

module pb2 (input i, output o);
  lfx u1 (.i(i), .o(o));
endmodule

module top (input a, input b, output x, output y);
  pa2 ua (.i(a), .o(x));
  pb2 ub (.i(b), .o(y));
endmodule
