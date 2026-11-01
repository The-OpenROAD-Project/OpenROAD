// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, positional_conn, segv_bracket
// CLUE: Bracket for the link_design crash: three explicit named header ports
// CLUE: (two in, one out) connected positionally.

module top (a, b, y);
 input a, b;
 output y;
 sub u (a, b, y);
endmodule

module sub (.px(m), .py(n), .pz(r));
 input m, n;
 output r;
 NAND2_X1 g (.A1(m), .A2(n), .ZN(r));
endmodule
