// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, output_first_header
// CLUE: Child header declares the output FIRST, so the first positional
// CLUE: argument is the output net.

module top (a, b, y);
 input a, b;
 output y;
 sub u (y, a, b);
endmodule

module sub (z, p, q);
 output z;
 input p, q;
 NOR2_X1 g (.A1(p), .A2(q), .ZN(z));
endmodule
