// TOP: top
// TECH: nangate45
// TARGETS: module_reuse, instantiated_by_top_and_submodule
// CLUE: module rr instantiated directly by top AND by a submodule; the two
// usage contexts must both survive round trip without uniquification clash.

module rr (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module holder (input a, output z);
  rr u (.a(a), .z(z));
endmodule

module top (input a, input b, output x, output y);
  rr u_top (.a(a), .z(x));
  holder h (.a(b), .z(y));
endmodule
