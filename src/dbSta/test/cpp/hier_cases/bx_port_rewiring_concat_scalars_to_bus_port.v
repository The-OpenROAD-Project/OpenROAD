// TOP: top
// TECH: nangate45
// TARGETS: concat_port_conn, scalar_to_bus
// CLUE: Four scalar top inputs concatenated into one 4-bit child input port;
// CLUE: child output bus split back out to four scalar top outputs.

module top (a, b, c, d, w, x, y, z);
 input a, b, c, d;
 output w, x, y, z;
 wire [3:0] o;
 pbuf u (.i({a,b,c,d}), .o(o));
 BUF_X1 g0 (.A(o[0]), .Z(w));
 BUF_X1 g1 (.A(o[1]), .Z(x));
 BUF_X1 g2 (.A(o[2]), .Z(y));
 BUF_X1 g3 (.A(o[3]), .Z(z));
endmodule

module pbuf (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[1]), .Z(o[0]));
 BUF_X1 b1 (.A(i[2]), .Z(o[1]));
 BUF_X1 b2 (.A(i[3]), .Z(o[2]));
 BUF_X1 b3 (.A(i[0]), .Z(o[3]));
endmodule
