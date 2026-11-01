// TARGETS: part_select_port_conn, nested_part_select, narrowing, depth_3
// CLUE: A part-select of a port is re-part-selected at every level, narrowing
// CLUE: 4 -> 3 -> 2 bits, so the surviving two bits are a[3] and a[4] of the
// CLUE: top input. Each drop must be tracked or the wrong bits arrive. The
// CLUE: outer bits a[0] and a[5] are consumed at top so no input dangles.

module top (a, y, z);
 input [5:0] a;
 output [1:0] y;
 output [1:0] z;
 l1 u (.i(a[4:1]), .o(y));
 INV_X1 t0 (.A(a[0]), .ZN(z[0]));
 BUF_X1 t1 (.A(a[5]), .Z(z[1]));
endmodule

module l1 (i, o);
 input [3:0] i;
 output [1:0] o;
 l2 u (.i(i[3:1]), .o(o));
endmodule

module l2 (i, o);
 input [2:0] i;
 output [1:0] o;
 l3 u (.i(i[2:1]), .o(o));
endmodule

module l3 (i, o);
 input [1:0] i;
 output [1:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
endmodule
