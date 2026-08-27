// TARGETS: hier_uniquify, clone_name_control, module_name, depth_2
// CLUE: control for the clone-name collisions.  makeUniqueDbModule
// (dbModule.cpp:595-619) builds <cell>_<inst>, so instances u1 and u2 of sub ask
// for sub_u1 and sub_u2.  A third, unrelated user module named sub_u3 is
// instantiated alongside them: it is one character away from the manufactured
// names and must keep its own identity and its own contents (INV, not BUF).
module sub (input a, output z);
  BUF_X1 g (.A(a), .Z(z));
endmodule

module sub_u3 (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module top (input i1, input i2, input i3, output o1, output o2, output o3);
  sub u1 (.a(i1), .z(o1));
  sub u2 (.a(i2), .z(o2));
  sub_u3 k (.a(i3), .z(o3));
endmodule
