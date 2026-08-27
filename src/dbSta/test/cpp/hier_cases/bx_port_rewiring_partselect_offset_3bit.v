// TOP: top
// TECH: nangate45
// TARGETS: part_select_port_conn, offset_slice
// CLUE: A 3-bit child port is fed by a[3:1] and drives y[2:0]: the slice is
// CLUE: offset, so port bit k corresponds to net bit k+1.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 h3 u (.i(a[3:1]), .o(y[2:0]));
 BUF_X1 g (.A(a[0]), .Z(y[3]));
endmodule

module h3 (i, o);
 input [2:0] i;
 output [2:0] o;
 INV_X1 b0 (.A(i[0]), .ZN(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 INV_X1 b2 (.A(i[2]), .ZN(o[2]));
endmodule
