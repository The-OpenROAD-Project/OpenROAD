// TOP: top
// TECH: nangate45
// TARGETS: perm, depth_1, two_instances, opposite_concat
// CLUE: One permuting module instantiated twice, once with a straight bus
// CLUE: binding and once with a reversed concat binding, driving two separate
// CLUE: output buses. Same master, contradictory bit maps.

module top (a, y, z);
 input [3:0] a;
 output [3:0] y;
 output [3:0] z;
 p1 u1 (.i(a), .o(y));
 p1 u2 (.i({a[0],a[1],a[2],a[3]}), .o(z));
endmodule

module p1 (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[1]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[3]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule
