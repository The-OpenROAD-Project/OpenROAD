// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, header_concat, scalars_to_bus
// CLUE: One external 2-bit port pk is the concatenation {b,a} of two internal
// CLUE: scalar inputs, so pk[1] is b and pk[0] is a.

module top (m, n, y);
 input m, n;
 output y;
 sub u (.pk({m,n}), .z(y));
endmodule

module sub (.pk({b,a}), .z(z));
 input a, b;
 output z;
 NAND2_X1 g (.A1(a), .A2(b), .ZN(z));
endmodule
