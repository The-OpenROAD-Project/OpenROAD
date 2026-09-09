// TOP: top
// TECH: nangate45
// TARGETS: perm_cancel, depth_3, bus, concat_port_conn, in_and_out_concat
// CLUE: rev applied by an input concat at level 1, rev applied by an OUTPUT
// CLUE: concat at level 2, straight buffers at the leaf: net identity spread
// CLUE: over three hierarchy levels.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 l1 u (.i(a), .o(y));
endmodule

module l1 (i, o);
 input [3:0] i;
 output [3:0] o;
 l2 u (.i({i[0],i[1],i[2],i[3]}), .o(o));
endmodule

module l2 (i, o);
 input [3:0] i;
 output [3:0] o;
 l3 u (.i(i), .o({o[0],o[1],o[2],o[3]}));
endmodule

module l3 (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule
