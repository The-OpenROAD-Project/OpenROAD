// TARGETS: two_instances, three_instances, part_select_port_conn, uniquify
// CLUE: One master instantiated three times, each getting a different 2-bit
// CLUE: part-select of the input and driving a different 2-bit part-select of
// CLUE: the output, with the input slice and output slice rotated relative to
// CLUE: each other. Three clones of one module must each keep their own
// CLUE: binding; the master itself also crosses its two bits.

module top (a, y);
 input [5:0] a;
 output [5:0] y;
 p u0 (.i(a[1:0]), .o(y[5:4]));
 p u1 (.i(a[3:2]), .o(y[1:0]));
 p u2 (.i(a[5:4]), .o(y[3:2]));
endmodule

module p (i, o);
 input [1:0] i;
 output [1:0] o;
 INV_X1 g0 (.A(i[1]), .ZN(o[0]));
 BUF_X1 g1 (.A(i[0]), .Z(o[1]));
endmodule
