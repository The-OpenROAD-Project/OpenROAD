// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, port_conn, extend
// CLUE: A 2-bit net bound to a 4-bit child input port: Verilog extends, so
// CLUE: port bits [3:2] are not driven from the parent. What does the reader
// CLUE: do with the unfilled port bits?

module top (a, y);
 input [1:0] a;
 output [3:0] y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule
