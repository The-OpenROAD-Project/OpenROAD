// TARGETS: positional_conn, nonansi_header_order, many_ports, depth_2
// CLUE: Twelve scalar ports connected POSITIONALLY, with the header listing all
// CLUE: six outputs before all six inputs while the body declares inputs first,
// CLUE: and the six input arguments handed over in reverse bus order. Nothing
// CLUE: about the mapping is recoverable from names, only from position.

module top (a, y);
 input [5:0] a;
 output [5:0] y;
 mid u (.i(a), .o(y));
endmodule

module mid (i, o);
 input [5:0] i;
 output [5:0] o;
 sub u (o[0], o[1], o[2], o[3], o[4], o[5],
        i[5], i[4], i[3], i[2], i[1], i[0]);
endmodule

module sub (q0, q1, q2, q3, q4, q5, p0, p1, p2, p3, p4, p5);
 input p0;
 input p1;
 input p2;
 input p3;
 input p4;
 input p5;
 output q0;
 output q1;
 output q2;
 output q3;
 output q4;
 output q5;
 BUF_X1 g0 (.A(p0), .Z(q0));
 INV_X1 g1 (.A(p1), .ZN(q1));
 BUF_X1 g2 (.A(p2), .Z(q2));
 INV_X1 g3 (.A(p3), .ZN(q3));
 BUF_X1 g4 (.A(p4), .Z(q4));
 INV_X1 g5 (.A(p5), .ZN(q5));
endmodule
