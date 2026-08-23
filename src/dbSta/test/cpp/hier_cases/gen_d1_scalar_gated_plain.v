// TOP: top
// TECH: nangate45
// TARGETS: depth_1, scalar, gated, naming_plain
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// scalar ports wired gated, plain identifiers.

module top (ti, to);
 input ti;
 output to;

 gen_d1_scalar_gated_plain_w0 u_top (.wi0(ti), .wo0(to));
endmodule

module gen_d1_scalar_gated_plain_w0 (wi0, wo0);
 input wi0;
 output wo0;

 gen_d1_scalar_gated_plain_leaf u_child (.li(wi0), .lo(wo0));
endmodule

module gen_d1_scalar_gated_plain_leaf (li, lo);
 input li;
 output lo;

 BUF_X1 g0 (.A(li), .Z(lo));
endmodule
