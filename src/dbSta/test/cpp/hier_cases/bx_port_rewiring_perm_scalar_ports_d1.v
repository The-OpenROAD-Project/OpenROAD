// TOP: top
// TECH: nangate45
// TARGETS: perm_rev, depth_1, scalar_ports
// CLUE: Same 4-bit reversal but through four SCALAR ports instead of a bus,
// CLUE: so the permutation lives entirely in scalar port bindings.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sperm u (.p0(a[0]), .p1(a[1]), .p2(a[2]), .p3(a[3]),
          .q0(y[0]), .q1(y[1]), .q2(y[2]), .q3(y[3]));
endmodule

module sperm (p0, p1, p2, p3, q0, q1, q2, q3);
 input p0, p1, p2, p3;
 output q0, q1, q2, q3;
 BUF_X1 b0 (.A(p3), .Z(q0));
 BUF_X1 b1 (.A(p2), .Z(q1));
 BUF_X1 b2 (.A(p1), .Z(q2));
 BUF_X1 b3 (.A(p0), .Z(q3));
endmodule
