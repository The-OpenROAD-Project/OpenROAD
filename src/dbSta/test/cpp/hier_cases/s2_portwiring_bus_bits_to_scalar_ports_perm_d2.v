// TARGETS: bus_bit_to_scalar_port, port_perm, depth_2
// CLUE: A 4-bit bus is taken apart bit by bit and handed to four SCALAR ports
// CLUE: of the grandchild in a scrambled order, and the four scalar results
// CLUE: are put back into a bus in a different scrambled order. Bus bit to
// CLUE: scalar port is the boundary where a bit name like i[2] must survive as
// CLUE: a net name on one side and a plain port name on the other.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 mid u (.i(a), .o(y));
endmodule

module mid (i, o);
 input [3:0] i;
 output [3:0] o;
 leaf u (.s3(i[0]), .s2(i[3]), .s1(i[1]), .s0(i[2]),
         .t3(o[1]), .t2(o[0]), .t1(o[3]), .t0(o[2]));
endmodule

module leaf (s3, s2, s1, s0, t3, t2, t1, t0);
 input s3;
 input s2;
 input s1;
 input s0;
 output t3;
 output t2;
 output t1;
 output t0;
 BUF_X1 g3 (.A(s3), .Z(t3));
 INV_X1 g2 (.A(s2), .ZN(t2));
 BUF_X1 g1 (.A(s1), .Z(t1));
 INV_X1 g0 (.A(s0), .ZN(t0));
endmodule
