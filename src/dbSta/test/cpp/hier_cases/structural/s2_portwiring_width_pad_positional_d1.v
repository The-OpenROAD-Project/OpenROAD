// TARGETS: port_width_mismatch, positional_conn, bus_pad, depth_1
// CLUE: Legal Verilog width mismatch on a POSITIONAL port connection: the 2-bit
// CLUE: net a is handed to a 4-bit child input port i, so i[1:0] = a[1:0] and
// CLUE: i[3:2] pad. The child never reads i[3:2] so no constant reaches an
// CLUE: output. The scalar pair (c -> w) is width-matched and stays live.

module top (a, c, y, w);
 input [1:0] a;
 input c;
 output [1:0] y;
 output w;
 sub u (a, c, y, w);
endmodule

module sub (i, s, o, q);
 input [3:0] i;
 input s;
 output [1:0] o;
 output q;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 INV_X1 b2 (.A(s), .ZN(q));
endmodule
