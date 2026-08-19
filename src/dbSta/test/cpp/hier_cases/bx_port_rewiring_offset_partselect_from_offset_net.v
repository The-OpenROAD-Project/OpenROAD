// TOP: top
// TECH: nangate45
// TARGETS: bus_index_base, offset_range, part_select_port_conn
// CLUE: A part-select w[6:5] of an offset net wire [7:4] feeds a [1:0] child
// CLUE: port: both the net base and the slice base are non-zero.

module top (a, y);
 input [3:0] a;
 output [1:0] y;
 wire [7:4] w;
 BUF_X1 f0 (.A(a[0]), .Z(w[4]));
 BUF_X1 f1 (.A(a[1]), .Z(w[5]));
 BUF_X1 f2 (.A(a[2]), .Z(w[6]));
 BUF_X1 f3 (.A(a[3]), .Z(w[7]));
 sub u (.i(w[6:5]), .o(y));
endmodule

module sub (i, o);
 input [1:0] i;
 output [1:0] o;
 INV_X1 b0 (.A(i[0]), .ZN(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
endmodule
