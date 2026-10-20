// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, bus_slice_port, crossed_halves
// CLUE: Two external scalar-pair ports lo/hi are slices of ONE internal 4-bit
// CLUE: input net; the parent feeds them crossed (lo<=a[3:2], hi<=a[1:0]).

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (.lo(a[3:2]), .hi(a[1:0]), .o(y));
endmodule

module sub (.lo(i[1:0]), .hi(i[3:2]), .o(o));
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule
