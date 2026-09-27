// TOP: top
// TECH: nangate45
// TARGETS: header_concat, unnamed_port, positional_conn, port_order
// CLUE: Bracket: the unnamed concatenation port is the SECOND port instead of
// CLUE: the first, to see whether the drop is position dependent.

module top (p, q, y);
 input p, q;
 output y;
 sub u (y, {p,q});
endmodule

module sub (z, {a,b});
 input a, b;
 output z;
 NAND2_X1 g (.A1(a), .A2(b), .ZN(z));
endmodule
