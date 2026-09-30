// TARGETS: range_direction_mismatch, whole_bus_connect, depth_3
// CLUE: The declared bus direction alternates at every one of the three
// CLUE: boundaries ([3:0] -> [0:3] -> [3:0] -> [0:3]) with plain whole-bus
// CLUE: connections, so each boundary is a positional reversal. The leaf
// CLUE: permutes and alternates BUF/INV so every output bit is a distinct
// CLUE: function of a distinct input bit.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 l1 u (.i(a), .o(y));
endmodule

module l1 (i, o);
 input [0:3] i;
 output [0:3] o;
 l2 u (.i(i), .o(o));
endmodule

module l2 (i, o);
 input [3:0] i;
 output [3:0] o;
 l3 u (.i(i), .o(o));
endmodule

module l3 (i, o);
 input [0:3] i;
 output [0:3] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[1]));
 INV_X1 b1 (.A(i[1]), .ZN(o[0]));
 BUF_X1 b2 (.A(i[2]), .Z(o[3]));
 INV_X1 b3 (.A(i[3]), .ZN(o[2]));
endmodule
