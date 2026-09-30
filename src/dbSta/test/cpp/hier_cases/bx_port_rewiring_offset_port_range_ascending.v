// TOP: top
// TECH: nangate45
// TARGETS: bus_index_base, offset_range, ascending_range
// CLUE: Child ports declared [4:7] (ascending AND offset) against [3:0] parent
// CLUE: nets: position 0 of the connection is port index 4, which is a[3].

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 input [4:7] i;
 output [4:7] o;
 INV_X1 b0 (.A(i[4]), .ZN(o[4]));
 INV_X1 b1 (.A(i[5]), .ZN(o[5]));
 INV_X1 b2 (.A(i[6]), .ZN(o[6]));
 INV_X1 b3 (.A(i[7]), .ZN(o[7]));
endmodule
