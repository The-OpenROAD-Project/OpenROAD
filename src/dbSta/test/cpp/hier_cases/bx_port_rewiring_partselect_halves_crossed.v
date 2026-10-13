// TOP: top
// TECH: nangate45
// TARGETS: part_select_port_conn, bus_halves
// CLUE: Two 2-bit children each get a part-select of the 4-bit top bus, but
// CLUE: the halves are crossed on the output side (upper in -> lower out).

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 h2 u1 (.i(a[3:2]), .o(y[1:0]));
 h2 u2 (.i(a[1:0]), .o(y[3:2]));
endmodule

module h2 (i, o);
 input [1:0] i;
 output [1:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 BUF_X1 b1 (.A(i[1]), .Z(o[1]));
endmodule
