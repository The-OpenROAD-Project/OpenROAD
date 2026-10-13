// TARGETS: port_width_mismatch, named_conn, bus_truncate, depth_1
// CLUE: Legal Verilog width mismatch on a NAMED port connection: the 4-bit net
// CLUE: a is handed to a 2-bit child input port i, so i[1:0] = a[1:0] and
// CLUE: a[3:2] is simply not carried in. a[2] and a[3] are consumed by top-level
// CLUE: gates so no top input bit is left dangling.

module top (a, y, z);
 input [3:0] a;
 output [1:0] y;
 output [1:0] z;
 sub u (.i(a), .o(y));
 INV_X1 t0 (.A(a[2]), .ZN(z[0]));
 BUF_X1 t1 (.A(a[3]), .Z(z[1]));
endmodule

module sub (i, o);
 input [1:0] i;
 output [1:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
endmodule
