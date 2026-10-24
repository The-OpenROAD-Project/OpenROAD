// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, empty_positional, dangling_in
// CLUE: Empty positional slot on a child INPUT port that is unused inside the
// CLUE: child, so nothing floats: purely a port-shape preservation question.

module top (a, y);
 input a;
 output y;
 sub u (a, , y);
endmodule

module sub (i, di, o);
 input i, di;
 output o;
 BUF_X1 b (.A(i), .Z(o));
endmodule
