// TOP: top
// TECH: nangate45
// TARGETS: fanout_16, two_level_replication, 4x4_instances
// CLUE: same leaf module 16x total but split 4 parents x 4 instances, and the
// parent type itself is instantiated 4 times; two levels of same-module
// replication at once.

module bc (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module quad (input [3:0] a, output [3:0] z);
  bc u0 (.a(a[0]), .z(z[0]));
  bc u1 (.a(a[1]), .z(z[1]));
  bc u2 (.a(a[2]), .z(z[2]));
  bc u3 (.a(a[3]), .z(z[3]));
endmodule

module top (input [15:0] a, output [15:0] z);
  quad q0 (.a(a[3:0]),   .z(z[3:0]));
  quad q1 (.a(a[7:4]),   .z(z[7:4]));
  quad q2 (.a(a[11:8]),  .z(z[11:8]));
  quad q3 (.a(a[15:12]), .z(z[15:12]));
endmodule
