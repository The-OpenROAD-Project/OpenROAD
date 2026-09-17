// TOP: top
// TECH: nangate45
// TARGETS: dff_split_3_modules, d_cone_and_q_cone_in_siblings
// CLUE: the D cone, the flop and the Q cone each live in a different
// submodule, so the register's fan-in and fan-out both cross a boundary.

module dcone (input a, input b, output d);
  XOR2_X1 g (.A(a), .B(b), .Z(d));
endmodule

module ffmod (input d, input ck, output q);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q));
endmodule

module qcone (input q, input c, output z);
  NAND2_X1 g (.A1(q), .A2(c), .ZN(z));
endmodule

module top (input a, input b, input c, input ck, output z);
  wire d, q;
  dcone u1 (.a(a), .b(b), .d(d));
  ffmod u2 (.d(d), .ck(ck), .q(q));
  qcone u3 (.q(q), .c(c), .z(z));
endmodule
