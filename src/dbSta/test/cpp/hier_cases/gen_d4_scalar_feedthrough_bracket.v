// TOP: top
// TECH: nangate45
// TARGETS: depth_4, scalar, feedthrough, naming_bracket
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 4 nesting,
// scalar ports wired feedthrough, bracket identifiers.

module top (\ti[0] , \to[0] );
 input \ti[0] ;
 output \to[0] ;

 gen_d4_scalar_feedthrough_bracket_w3 \u_top[0]  (.\wi3[0] (\ti[0] ), .\wo3[0] (\to[0] ));
endmodule

module gen_d4_scalar_feedthrough_bracket_w3 (\wi3[0] , \wo3[0] );
 input \wi3[0] ;
 output \wo3[0] ;

 gen_d4_scalar_feedthrough_bracket_w2 \u_child[0]  (.\wi2[0] (\wi3[0] ), .\wo2[0] (\wo3[0] ));
endmodule

module gen_d4_scalar_feedthrough_bracket_w2 (\wi2[0] , \wo2[0] );
 input \wi2[0] ;
 output \wo2[0] ;

 gen_d4_scalar_feedthrough_bracket_w1 \u_child[0]  (.\wi1[0] (\wi2[0] ), .\wo1[0] (\wo2[0] ));
endmodule

module gen_d4_scalar_feedthrough_bracket_w1 (\wi1[0] , \wo1[0] );
 input \wi1[0] ;
 output \wo1[0] ;

 gen_d4_scalar_feedthrough_bracket_w0 \u_child[0]  (.\wi0[0] (\wi1[0] ), .\wo0[0] (\wo1[0] ));
endmodule

module gen_d4_scalar_feedthrough_bracket_w0 (\wi0[0] , \wo0[0] );
 input \wi0[0] ;
 output \wo0[0] ;

 gen_d4_scalar_feedthrough_bracket_leaf \u_child[0]  (.\li[0] (\wi0[0] ), .\lo[0] (\wo0[0] ));
endmodule

module gen_d4_scalar_feedthrough_bracket_leaf (\li[0] , \lo[0] );
 input \li[0] ;
 output \lo[0] ;

 assign \lo[0]  = \li[0] ;
endmodule
