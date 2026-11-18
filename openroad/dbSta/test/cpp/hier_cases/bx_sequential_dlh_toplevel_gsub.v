// TOP: top
// TECH: nangate45
// TARGETS: dlh_latch_at_top, d_and_gate_enable_from_submodules
// CLUE: a DLH_X1 level-sensitive latch at the top level with both its D and
// its transparency enable G computed in different submodules.

module dcone (input a, input b, output z);
  XOR2_X1 g (.A(a), .B(b), .Z(z));
endmodule

module gcone (input a, input b, output z);
  NAND2_X1 g (.A1(a), .A2(b), .ZN(z));
endmodule

module top (input a, input b, input c, output z);
  wire d, en, q;
  dcone u0 (.a(a), .b(b), .z(d));
  gcone u1 (.a(b), .b(c), .z(en));
  DLH_X1 lat (.D(d), .G(en), .Q(q));
  assign z = q;
endmodule
