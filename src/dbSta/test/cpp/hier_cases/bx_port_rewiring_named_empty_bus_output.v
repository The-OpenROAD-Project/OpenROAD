// TOP: top
// TECH: nangate45
// TARGETS: named_conn, empty_port_conn, bus, dangling_out
// CLUE: .o() explicitly leaves a 4-bit child output port unconnected while it
// CLUE: is fully driven inside the child.

module top (a, y);
 input a;
 output y;
 sub u (.k(a), .o(), .z(y));
endmodule

module sub (k, o, z);
 input k;
 output [3:0] o;
 output z;
 INV_X1 b0 (.A(k), .ZN(o[0]));
 INV_X1 b1 (.A(k), .ZN(o[1]));
 INV_X1 b2 (.A(k), .ZN(o[2]));
 INV_X1 b3 (.A(k), .ZN(o[3]));
 BUF_X1 g (.A(k), .Z(z));
endmodule
