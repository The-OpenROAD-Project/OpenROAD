// TARGETS: bus_width1, offset_bus_range, bus_bit_to_bus_port, depth_2
// CLUE: A width-1 bus port declared with a NON-ZERO offset, [5:5] in and [1:1]
// CLUE: out, fed from a plain bit-select of the parent bus. from==to makes the
// CLUE: updown test degenerate and the single member has to be found at
// CLUE: offset 0 of a range that does not start at 0.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 mid u (.i(a), .o(y));
endmodule

module mid (i, o);
 input [3:0] i;
 output [3:0] o;
 leaf u2 (.b(i[2]), .c(o[3]));
 BUF_X1 g0 (.A(i[0]), .Z(o[0]));
 INV_X1 g1 (.A(i[1]), .ZN(o[1]));
 BUF_X1 g3 (.A(i[3]), .Z(o[2]));
endmodule

module leaf (b, c);
 input [5:5] b;
 output [1:1] c;
 INV_X1 g (.A(b[5]), .ZN(c[1]));
endmodule
