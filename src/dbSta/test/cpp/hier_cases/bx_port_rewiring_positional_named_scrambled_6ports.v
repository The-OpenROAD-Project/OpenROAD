// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, named_conn, nonansi_header_order, two_instances
// CLUE: A 6-port child whose non-ANSI header order is unrelated to its
// CLUE: declaration order is instantiated twice: u1 positionally, u2 by name,
// CLUE: with the roles of two inputs swapped between them.

module top (a, b, c, y0, y1, y2, y3);
 input a, b, c;
 output y0, y1, y2, y3;
 sub u1 (y0, a, b, c, y1, c);
 sub u2 (.q0(y2), .p0(b), .p1(a), .p2(c), .q1(y3), .p3(c));
endmodule

module sub (q0, p0, p1, p2, q1, p3);
 output q1;
 input p1, p3;
 output q0;
 input p0, p2;
 AOI21_X1 g0 (.A(p0), .B1(p1), .B2(p2), .ZN(q0));
 OAI21_X1 g1 (.A(p1), .B1(p2), .B2(p3), .ZN(q1));
endmodule
