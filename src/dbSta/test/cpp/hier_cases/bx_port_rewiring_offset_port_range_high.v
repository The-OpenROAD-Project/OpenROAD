// TOP: top
// TECH: nangate45
// TARGETS: bus_index_base, offset_range, perm_rot1
// CLUE: The child's ports are declared [7:4] while the parent nets are [3:0].
// CLUE: The vector connection is by position, so a[3] lands on port index 7.
// CLUE: A reader that keys bus bits by absolute index corrupts the mapping.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 input [7:4] i;
 output [7:4] o;
 BUF_X1 b0 (.A(i[5]), .Z(o[4]));
 BUF_X1 b1 (.A(i[6]), .Z(o[5]));
 BUF_X1 b2 (.A(i[7]), .Z(o[6]));
 BUF_X1 b3 (.A(i[4]), .Z(o[7]));
endmodule
