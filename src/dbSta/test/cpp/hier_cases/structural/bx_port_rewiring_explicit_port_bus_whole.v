// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, bus, name_decoupled
// CLUE: Bus version of the explicit-named-port rename: external ports pi/po are
// CLUE: whole 4-bit internal nets i/o, and the child rotates the bus.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (.pi(a), .po(y));
endmodule

module sub (.pi(i), .po(o));
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[1]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[3]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule
