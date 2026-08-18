// TARGETS: output_port_readback, concat_port_conn, depth_2
// CLUE: A module reads its own OUTPUT bus port back and feeds it, crossed by a
// CLUE: concat, into a grandchild's input port. The same modbterm is therefore
// CLUE: a boundary term going up and a driver going down, and the concat means
// CLUE: losing the crossing changes the value at z.

module top (a, y, z);
 input [1:0] a;
 output [1:0] y;
 output [1:0] z;
 mid u (.i(a), .o(y), .q(z));
endmodule

module mid (i, o, q);
 input [1:0] i;
 output [1:0] o;
 output [1:0] q;
 INV_X1 g0 (.A(i[0]), .ZN(o[0]));
 BUF_X1 g1 (.A(i[1]), .Z(o[1]));
 leaf u (.i({o[0],o[1]}), .o(q));
endmodule

module leaf (i, o);
 input [1:0] i;
 output [1:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
endmodule
