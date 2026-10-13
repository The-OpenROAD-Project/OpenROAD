// TOP: top
// TECH: nangate45
// TARGETS: perm_rot1, depth_4, composed_perm
// CLUE: A one-position rotate applied in the port connection at each of 4
// CLUE: nested levels: the composed effect is a rotate by 4 on a 4-bit bus,
// CLUE: i.e. the identity, but every level must be tracked to get there.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 l1 u (.i(a), .o(y));
endmodule

module l1 (i, o);
 input [3:0] i;
 output [3:0] o;
 l2 u (.i({i[0],i[3],i[2],i[1]}), .o(o));
endmodule

module l2 (i, o);
 input [3:0] i;
 output [3:0] o;
 l3 u (.i({i[0],i[3],i[2],i[1]}), .o(o));
endmodule

module l3 (i, o);
 input [3:0] i;
 output [3:0] o;
 l4 u (.i({i[0],i[3],i[2],i[1]}), .o(o));
endmodule

module l4 (i, o);
 input [3:0] i;
 output [3:0] o;
 leaf u (.i({i[0],i[3],i[2],i[1]}), .o(o));
endmodule

module leaf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule
