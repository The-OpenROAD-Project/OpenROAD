// TOP: top
// TECH: nangate45
// TARGETS: bus_index_base, offset_range, parent_net_offset
// CLUE: The PARENT's intermediate net is wire [7:4] w, filled from the top
// CLUE: input in scrambled order, then handed whole-vector to a [3:0] child
// CLUE: port.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 wire [7:4] w;
 BUF_X1 f0 (.A(a[2]), .Z(w[4]));
 BUF_X1 f1 (.A(a[3]), .Z(w[5]));
 BUF_X1 f2 (.A(a[0]), .Z(w[6]));
 BUF_X1 f3 (.A(a[1]), .Z(w[7]));
 sub u (.i(w), .o(y));
endmodule

module sub (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule
