// TOP: top
// TECH: nangate45
// TARGETS: module_name_prefix_trap, sub_sub_1_sub2_exist, different_tieoffs
// CLUE: modules sub, sub_1, sub2 all exist AND sub is instantiated twice with
// different constant tie-offs (so the two instance contexts differ). If the
// tool uniquifies sub into sub_1 it collides with the real sub_1.

module sub (input i, input j, output o);
  NAND2_X1 g (.A1(i), .A2(j), .ZN(o));
endmodule

module sub_1 (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module sub2 (input i, output o);
  BUF_X1 g (.A(i), .Z(o));
endmodule

module top (input a, input b, output w, output x, output y, output z);
  wire one, zero;
  LOGIC1_X1 t1 (.Z(one));
  LOGIC0_X1 t0 (.Z(zero));
  sub  s1 (.i(a), .j(one),  .o(w));
  sub  s2 (.i(b), .j(zero), .o(x));
  sub_1 s3 (.i(a), .o(y));
  sub2  s4 (.i(b), .o(z));
endmodule
