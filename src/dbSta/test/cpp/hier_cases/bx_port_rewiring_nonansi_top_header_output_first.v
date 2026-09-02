// TOP: top
// TECH: nangate45
// TARGETS: nonansi_header_order, top_ports, output_first_header
// CLUE: The TOP module lists its output before its input and declares them in
// CLUE: the opposite order: top port order is the thing under test.

module top (y, a);
 input [3:0] a;
 output [3:0] y;
 pbuf u (.i(a), .o(y));
endmodule

module pbuf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule
