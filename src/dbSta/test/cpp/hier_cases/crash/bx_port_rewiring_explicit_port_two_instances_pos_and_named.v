// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, positional_conn, named_conn, two_instances
// CLUE: Bracket: the same explicit-port module instantiated twice, once by
// CLUE: external port name and once positionally, to confirm which binding form
// CLUE: triggers the failure.

module top (a, y0, y1);
 input a;
 output y0, y1;
 sub u1 (.px(a), .py(y0));
 sub u2 (a, y1);
endmodule

module sub (.px(m), .py(r));
 input m;
 output r;
 INV_X1 g (.A(m), .ZN(r));
endmodule
