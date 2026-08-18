// TOP: top
// TECH: nangate45
// TARGETS: assign_feedthrough, two_bus_pairs, cross_assign
// CLUE: PROBE around known finding 2: the child has two input buses and two
// CLUE: output buses and crosses them with whole-vector assigns.

module top (a, b, x, y);
 input [1:0] a, b;
 output [1:0] x, y;
 sw u (.i1(a), .i2(b), .o1(x), .o2(y));
endmodule

module sw (i1, i2, o1, o2);
 input [1:0] i1, i2;
 output [1:0] o1, o2;
 assign o1 = i2;
 assign o2 = i1;
endmodule
