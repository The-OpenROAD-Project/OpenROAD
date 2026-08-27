// TARGETS: flat_path_collision, implicit_net, declaration_order, depth_2
// CLUE: same collision as the declared-wire cases but the victim is an IMPLICIT
// net -- \x/w is never declared, it only appears on a gate terminal -- and the
// declaration order is flipped: top comes first in the file and the escaped name
// is first used AFTER the hierarchical instance.  If the reader creates implicit
// nets by a different route than declared ones, the flat name it synthesizes for
// net w of instance x (dbReadVerilog.cc:732) could land on the other side of the
// collision, or be resolved in the other direction.
module top (input i1, input i2, output o1, output o2);
  subw x (.a(i1), .z(o1));
  NAND2_X1 g3 (.A1(i2), .A2(i1), .ZN(\x/w ));
  BUF_X1 g4 (.A(\x/w ), .Z(o2));
endmodule

module subw (input a, output z);
  wire w;
  INV_X1 g1 (.A(a), .ZN(w));
  INV_X1 g2 (.A(w), .ZN(z));
endmodule
