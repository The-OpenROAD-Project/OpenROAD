// TARGETS: positional_conn, short_port_list, sub_output_dangling, depth_2
// CLUE: A POSITIONAL connection list with fewer arguments than the child has
// CLUE: ports -- three args for four ports -- which is legal Verilog and leaves
// CLUE: the trailing port unconnected. Unlike the (a, , b) gap form this is not
// CLUE: a syntax question; the binder simply has to stop at the shorter list.

module top (a, b, y, z);
 input a;
 input b;
 output y;
 output z;
 mid u (.p(a), .q(b), .r(y), .s(z));
endmodule

module mid (p, q, r, s);
 input p;
 input q;
 output r;
 output s;
 sub u (p, q, r);
 BUF_X1 g (.A(q), .Z(s));
endmodule

module sub (p0, p1, q0, q1);
 input p0;
 input p1;
 output q0;
 output q1;
 XOR2_X1 g0 (.A(p0), .B(p1), .Z(q0));
 INV_X1 g1 (.A(p0), .ZN(q1));
endmodule
