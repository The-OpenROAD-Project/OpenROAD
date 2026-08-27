// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, plain_port, positional_conn, segv_bracket
// CLUE: Bracket: header mixes one explicit named port with one plain port -
// CLUE: module sub (.px(m), r); - and the parent connects positionally.

module top (a, y);
 input a;
 output y;
 sub u (a, y);
endmodule

module sub (.px(m), r);
 input m;
 output r;
 INV_X1 g (.A(m), .ZN(r));
endmodule
