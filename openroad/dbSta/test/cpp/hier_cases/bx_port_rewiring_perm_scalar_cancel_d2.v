// TOP: top
// TECH: nangate45
// TARGETS: perm_cancel, depth_2, scalar_ports, cross_wired_outputs
// CLUE: Level 1 cross-wires the inner instance's OUTPUT ports (q0<->q3,
// CLUE: q1<->q2) while the inner module reverses in its body: identity.
// CLUE: Cancellation is split between an output binding and cell wiring.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 outer u (.p0(a[0]), .p1(a[1]), .p2(a[2]), .p3(a[3]),
          .q0(y[0]), .q1(y[1]), .q2(y[2]), .q3(y[3]));
endmodule

module outer (p0, p1, p2, p3, q0, q1, q2, q3);
 input p0, p1, p2, p3;
 output q0, q1, q2, q3;
 inner u (.p0(p0), .p1(p1), .p2(p2), .p3(p3),
          .q0(q3), .q1(q2), .q2(q1), .q3(q0));
endmodule

module inner (p0, p1, p2, p3, q0, q1, q2, q3);
 input p0, p1, p2, p3;
 output q0, q1, q2, q3;
 BUF_X1 b0 (.A(p3), .Z(q0));
 BUF_X1 b1 (.A(p2), .Z(q1));
 BUF_X1 b2 (.A(p1), .Z(q2));
 BUF_X1 b3 (.A(p0), .Z(q3));
endmodule
