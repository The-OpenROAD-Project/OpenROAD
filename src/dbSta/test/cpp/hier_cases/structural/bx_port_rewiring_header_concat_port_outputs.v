// TOP: top
// TECH: nangate45
// TARGETS: header_concat, unnamed_port, output_concat
// CLUE: Bracket: the unnamed concatenation port groups two OUTPUTS, so if it is
// CLUE: dropped the parent loses two drivers instead of two loads.

module top (a, y0, y1);
 input a;
 output y0, y1;
 sub u (a, {y1,y0});
endmodule

module sub (k, {o1,o0});
 input k;
 output o0, o1;
 INV_X1 g0 (.A(k), .ZN(o0));
 BUF_X1 g1 (.A(k), .Z(o1));
endmodule
