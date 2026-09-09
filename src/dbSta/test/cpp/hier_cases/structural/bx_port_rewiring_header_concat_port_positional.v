// TOP: top
// TECH: nangate45
// TARGETS: header_concat, unnamed_port, positional_conn
// CLUE: The child's FIRST port is an unnamed 2-bit concatenation {a,b} of two
// CLUE: internal scalars, so it can only be connected positionally.

module top (p, q, y);
 input p, q;
 output y;
 sub u ({p,q}, y);
endmodule

module sub ({a,b}, z);
 input a, b;
 output z;
 NAND2_X1 g (.A1(a), .A2(b), .ZN(z));
endmodule
