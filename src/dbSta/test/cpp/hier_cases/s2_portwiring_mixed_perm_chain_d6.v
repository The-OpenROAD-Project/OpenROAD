// TARGETS: concat_port_conn, composed_perm, depth_6
// CLUE: Six levels deep, and every boundary applies a DIFFERENT bijection of the
// CLUE: four bits, so nothing cancels and every level has to be tracked. The
// CLUE: output side is straight, which isolates the input-side composition.

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
 l3 u (.i({i[1],i[0],i[3],i[2]}), .o(o));
endmodule

module l3 (i, o);
 input [3:0] i;
 output [3:0] o;
 l4 u (.i({i[2],i[3],i[0],i[1]}), .o(o));
endmodule

module l4 (i, o);
 input [3:0] i;
 output [3:0] o;
 l5 u (.i({i[0],i[1],i[2],i[3]}), .o(o));
endmodule

module l5 (i, o);
 input [3:0] i;
 output [3:0] o;
 l6 u (.i({i[3],i[1],i[2],i[0]}), .o(o));
endmodule

module l6 (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 INV_X1 b3 (.A(i[3]), .ZN(o[3]));
endmodule
