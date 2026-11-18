// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, port_conn, truncate
// CLUE: A 4-bit net bound to a 2-bit child input port: the upper net bits are
// CLUE: truncated away by the port connection.

module top (a, y);
 input [3:0] a;
 output [1:0] y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 input [1:0] i;
 output [1:0] o;
 INV_X1 b0 (.A(i[0]), .ZN(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
endmodule
