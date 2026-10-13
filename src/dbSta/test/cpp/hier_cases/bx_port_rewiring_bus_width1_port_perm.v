// TOP: top
// TECH: nangate45
// TARGETS: bus_width1, port_conn, perm_swap
// CLUE: Both child ports are declared as WIDTH-1 BUSES ([0:0]) while the parent
// CLUE: nets are plain scalars, and the two single-bit buses are crossed.

module top (a, b, y0, y1);
 input a, b;
 output y0, y1;
 sub u (.i0(b), .i1(a), .o0(y0), .o1(y1));
endmodule

module sub (i0, i1, o0, o1);
 input [0:0] i0;
 input [0:0] i1;
 output [0:0] o0;
 output [0:0] o1;
 INV_X1 g0 (.A(i0[0]), .ZN(o0[0]));
 BUF_X1 g1 (.A(i1[0]), .Z(o1[0]));
endmodule
