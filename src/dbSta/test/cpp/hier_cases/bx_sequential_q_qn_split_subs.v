// TOP: top
// TECH: nangate45
// TARGETS: q_and_qn_consumed_in_different_submodules
// CLUE: the flop lives in one submodule, its Q is consumed by a second and
// its QN by a third -- both polarities cross boundaries independently.

module ffmod (input d, input ck, output q, output qn);
  DFF_X1 ff (.D(d), .CK(ck), .Q(q), .QN(qn));
endmodule

module useq (input a, input b, output z);
  AND2_X1 g (.A1(a), .A2(b), .ZN(z));
endmodule

module useqn (input a, input b, output z);
  OR2_X1 g (.A1(a), .A2(b), .ZN(z));
endmodule

module top (input d, input ck, input c, output z0, output z1);
  wire q, qn;
  ffmod u0 (.d(d), .ck(ck), .q(q), .qn(qn));
  useq  u1 (.a(q),  .b(c), .z(z0));
  useqn u2 (.a(qn), .b(c), .z(z1));
endmodule
