// TARGETS: named_conn, port_omitted, sub_output_dangling, depth_3
// CLUE: The grandchild's output port p is not mentioned AT ALL in the named
// CLUE: connection list -- not even as .p() -- so the port exists on the master
// CLUE: but has no entry to bind. The live cone through o runs beside it at
// CLUE: depth 3.

module top (a, y, z);
 input [1:0] a;
 output y;
 output z;
 l1 u (.i(a), .o(y), .p(z));
endmodule

module l1 (i, o, p);
 input [1:0] i;
 output o;
 output p;
 l2 u (.i(i), .o(o), .p(p));
endmodule

module l2 (i, o, p);
 input [1:0] i;
 output o;
 output p;
 leaf u (.i(i), .o(o));
 INV_X1 g (.A(i[1]), .ZN(p));
endmodule

module leaf (i, o, p);
 input [1:0] i;
 output o;
 output p;
 XOR2_X1 g0 (.A(i[0]), .B(i[1]), .Z(o));
 BUF_X1 g1 (.A(i[0]), .Z(p));
endmodule
