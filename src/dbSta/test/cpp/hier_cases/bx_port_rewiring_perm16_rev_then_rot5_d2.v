// TOP: top
// TECH: nangate45
// TARGETS: perm_rev, perm_rot, bus_16, depth_2
// CLUE: 16-bit bus: level 1 reverses it in the port concat, the leaf rotates by
// CLUE: 5 in its body. Wide buses stress bit-by-bit boundary bookkeeping.

module top (a, y);
 input [15:0] a;
 output [15:0] y;
 l1 u (.i(a), .o(y));
endmodule

module l1 (i, o);
 input [15:0] i;
 output [15:0] o;
 l2 u (.i({i[0],i[1],i[2],i[3],i[4],i[5],i[6],i[7],i[8],i[9],i[10],i[11],i[12],i[13],i[14],i[15]}), .o(o));
endmodule

module l2 (i, o);
 input [15:0] i;
 output [15:0] o;
 BUF_X1 b0 (.A(i[5]), .Z(o[0]));
 BUF_X1 b1 (.A(i[6]), .Z(o[1]));
 BUF_X1 b2 (.A(i[7]), .Z(o[2]));
 BUF_X1 b3 (.A(i[8]), .Z(o[3]));
 BUF_X1 b4 (.A(i[9]), .Z(o[4]));
 BUF_X1 b5 (.A(i[10]), .Z(o[5]));
 BUF_X1 b6 (.A(i[11]), .Z(o[6]));
 BUF_X1 b7 (.A(i[12]), .Z(o[7]));
 BUF_X1 b8 (.A(i[13]), .Z(o[8]));
 BUF_X1 b9 (.A(i[14]), .Z(o[9]));
 BUF_X1 b10 (.A(i[15]), .Z(o[10]));
 BUF_X1 b11 (.A(i[0]), .Z(o[11]));
 BUF_X1 b12 (.A(i[1]), .Z(o[12]));
 BUF_X1 b13 (.A(i[2]), .Z(o[13]));
 BUF_X1 b14 (.A(i[3]), .Z(o[14]));
 BUF_X1 b15 (.A(i[4]), .Z(o[15]));
endmodule
