// TOP: top
// TECH: nangate45
// TARGETS: range_mismatch, bus, depth_1
// CLUE: Parent net is [3:0] but the child port is declared [0:3]; the vector
// CLUE: connection matches by position, so a[3] lands on port index 0.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 input [0:3] i;
 output [0:3] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
 BUF_X1 b2 (.A(i[2]), .Z(o[2]));
 BUF_X1 b3 (.A(i[3]), .Z(o[3]));
endmodule
