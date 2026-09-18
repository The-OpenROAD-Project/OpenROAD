// TOP: top
// TECH: nangate45
// TARGETS: module_name_prefix_trap, sub_and_sub_1_exist, sub_twice
// CLUE: modules sub and sub_1 both exist; sub is instantiated twice. If hier
// uniquification renames the second sub instance's module to sub_1 it
// collides with the existing distinct module sub_1.

module sub (input i, output o);
  NAND2_X1 g (.A1(i), .A2(i), .ZN(o));
endmodule

module sub_1 (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module top (input a, input b, input c, output x, output y, output z);
  sub s1 (.i(a), .o(x));
  sub s2 (.i(b), .o(y));
  sub_1 s3 (.i(c), .o(z));
endmodule
