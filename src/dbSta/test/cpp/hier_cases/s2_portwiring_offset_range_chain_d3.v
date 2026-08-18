// TARGETS: offset_bus_range, whole_bus_connect, depth_3
// CLUE: The same 4-bit value passes through three submodule ports whose bus
// CLUE: ranges have three different non-zero offsets ([7:4], [11:8], [3:0]).
// CLUE: Binding is positional, so bit 3 of the top bus must land on index 7,
// CLUE: then 11, then 3. Any code that treats the index as the position breaks.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 l1 u (.i(a), .o(y));
endmodule

module l1 (i, o);
 input [7:4] i;
 output [7:4] o;
 l2 u (.i(i), .o(o));
endmodule

module l2 (i, o);
 input [11:8] i;
 output [11:8] o;
 l3 u (.i(i), .o(o));
endmodule

module l3 (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 g3 (.A(i[3]), .Z(o[2]));
 INV_X1 g2 (.A(i[2]), .ZN(o[3]));
 BUF_X1 g1 (.A(i[1]), .Z(o[0]));
 INV_X1 g0 (.A(i[0]), .ZN(o[1]));
endmodule
