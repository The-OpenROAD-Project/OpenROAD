// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, positional_conn, name_decoupled
// CLUE: Explicit named header ports px/py bound to internal nets b/a, connected
// CLUE: POSITIONALLY by the parent so only the header's port ORDER matters.

module top (a, y);
 input a;
 output y;
 sub u (a, y);
endmodule

module sub (.px(b), .py(a));
 input b;
 output a;
 INV_X1 g (.A(b), .ZN(a));
endmodule
