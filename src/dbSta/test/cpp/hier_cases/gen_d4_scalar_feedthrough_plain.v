// TOP: top
// TECH: nangate45
// TARGETS: depth_4, scalar, feedthrough, naming_plain
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 4 nesting,
// scalar ports wired feedthrough, plain identifiers.

module top (ti, to);
 input ti;
 output to;

 gen_d4_scalar_feedthrough_plain_w3 u_top (.wi3(ti), .wo3(to));
endmodule

module gen_d4_scalar_feedthrough_plain_w3 (wi3, wo3);
 input wi3;
 output wo3;

 gen_d4_scalar_feedthrough_plain_w2 u_child (.wi2(wi3), .wo2(wo3));
endmodule

module gen_d4_scalar_feedthrough_plain_w2 (wi2, wo2);
 input wi2;
 output wo2;

 gen_d4_scalar_feedthrough_plain_w1 u_child (.wi1(wi2), .wo1(wo2));
endmodule

module gen_d4_scalar_feedthrough_plain_w1 (wi1, wo1);
 input wi1;
 output wo1;

 gen_d4_scalar_feedthrough_plain_w0 u_child (.wi0(wi1), .wo0(wo1));
endmodule

module gen_d4_scalar_feedthrough_plain_w0 (wi0, wo0);
 input wi0;
 output wo0;

 gen_d4_scalar_feedthrough_plain_leaf u_child (.li(wi0), .lo(wo0));
endmodule

module gen_d4_scalar_feedthrough_plain_leaf (li, lo);
 input li;
 output lo;

 assign lo = li;
endmodule
