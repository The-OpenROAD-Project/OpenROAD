// TOP: top
// TECH: nangate45
// TARGETS: perm_cancel, depth_2, bus, concat_port_conn
// CLUE: Level 1 reverses the 4-bit bus in the port connection (concat), the
// CLUE: leaf reverses it again in its body: net effect is the identity.
// CLUE: Writer must keep both reversals, not collapse to a pass-through.

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
 BUF_X1 b0 (.A(i[3]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[1]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule
