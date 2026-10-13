// TOP: top
// TECH: nangate45
// TARGETS: pure_feedthrough_module_between_two_flops, scalar_assign
// CLUE: a logic-free module whose only content is `assign z = a;` sits between
// two flops; if that assign is dropped the register chain is cut.

module ft (input a, output z);
  assign z = a;
endmodule

module ffm (input d, input ck, input rn, output q);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q));
endmodule

module top (input d, input ck, input rn, output q);
  wire q0, q0f;
  ffm u0 (.d(d), .ck(ck), .rn(rn), .q(q0));
  ft  u1 (.a(q0), .z(q0f));
  ffm u2 (.d(q0f), .ck(ck), .rn(rn), .q(q));
endmodule
