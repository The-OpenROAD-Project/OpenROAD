// TARGETS: inout, bus_bit_split, two_instances, depth_2
// CLUE: The two bits of an INOUT bus are handed to two separate instances of a
// CLUE: scalar-inout leaf, crossed, one level below the module that owns the
// CLUE: bus port. Splitting an inout bus bit by bit across sibling instances
// CLUE: is the shape a bidirectional boundary is least likely to survive.

module top (a, io, y);
 input [1:0] a;
 inout [1:0] io;
 output [1:0] y;
 mid u (.p(a), .t(io), .z(y));
endmodule

module mid (p, t, z);
 input [1:0] p;
 inout [1:0] t;
 output [1:0] z;
 leaf u0 (.p(p[0]), .t(t[1]), .z(z[0]));
 leaf u1 (.p(p[1]), .t(t[0]), .z(z[1]));
endmodule

module leaf (p, t, z);
 input p;
 inout t;
 output z;
 XOR2_X1 g (.A(p), .B(t), .Z(z));
endmodule
