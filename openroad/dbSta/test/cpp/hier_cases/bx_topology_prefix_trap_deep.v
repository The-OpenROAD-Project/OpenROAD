// TOP: top
// TECH: nangate45
// TARGETS: module_name_prefix_trap, duplicated_inside_submodule
// CLUE: sub is instantiated twice INSIDE a wrapper module (not at top) while
// a distinct module sub_1 also exists at top level; uniquification triggered
// one level down must still avoid the sub_1 name.

module sub (input i, output o);
  NAND2_X1 g (.A1(i), .A2(i), .ZN(o));
endmodule

module sub_1 (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module wrap (input i1, input i2, output o1, output o2);
  sub a (.i(i1), .o(o1));
  sub b (.i(i2), .o(o2));
endmodule

module top (input a, input b, input c, output x, output y, output z);
  wrap w (.i1(a), .i2(b), .o1(x), .o2(y));
  sub_1 s (.i(c), .o(z));
endmodule
