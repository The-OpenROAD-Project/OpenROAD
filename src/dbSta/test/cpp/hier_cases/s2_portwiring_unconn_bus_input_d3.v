// TARGETS: sub_bus_input_unconnected, named_empty_bus, depth_3
// CLUE: A 2-bit INPUT bus port left explicitly empty (.j()) on an instance at
// CLUE: depth 3, while the same instance's other bus ports stay live. The
// CLUE: empty bus has to survive as a declared port of the master without
// CLUE: taking the live bits' bindings with it.

module top (a, y, z);
 input [1:0] a;
 output [1:0] y;
 output [1:0] z;
 l1 u (.i(a), .o(y), .q(z));
endmodule

module l1 (i, o, q);
 input [1:0] i;
 output [1:0] o;
 output [1:0] q;
 l2 u (.i(i), .o(o), .q(q));
endmodule

module l2 (i, o, q);
 input [1:0] i;
 output [1:0] o;
 output [1:0] q;
 leaf u (.i(i), .j(), .o(o));
 BUF_X1 g0 (.A(i[0]), .Z(q[0]));
 INV_X1 g1 (.A(i[1]), .ZN(q[1]));
endmodule

module leaf (i, j, o);
 input [1:0] i;
 input [1:0] j;
 output [1:0] o;
 INV_X1 b0 (.A(i[0]), .ZN(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
endmodule
