// TOP: top
// TECH: nangate45
// TARGETS: bus_width1, part_select_port_conn
// CLUE: A width-1 part-select a[2:2] feeds a scalar child port and a[1:1]
// CLUE: feeds a [0:0] bus child port: single-bit slices on both port shapes.

module top (a, y);
 input [3:0] a;
 output [1:0] y;
 sub u (.s(a[2:2]), .v(a[1:1]), .o(y));
endmodule

module sub (s, v, o);
 input s;
 input [0:0] v;
 output [1:0] o;
 INV_X1 g0 (.A(s), .ZN(o[0]));
 INV_X1 g1 (.A(v[0]), .ZN(o[1]));
endmodule
