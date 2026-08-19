// TOP: top
// TECH: nangate45
// TARGETS: depth_1, scalar, feedthrough, naming_bracket
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// scalar ports wired feedthrough, bracket identifiers.

module top (\ti[0] , \to[0] );
 input \ti[0] ;
 output \to[0] ;

 gen_d1_scalar_feedthrough_bracket_w0 \u_top[0]  (.\wi0[0] (\ti[0] ), .\wo0[0] (\to[0] ));
endmodule

module gen_d1_scalar_feedthrough_bracket_w0 (\wi0[0] , \wo0[0] );
 input \wi0[0] ;
 output \wo0[0] ;

 gen_d1_scalar_feedthrough_bracket_leaf \u_child[0]  (.\li[0] (\wi0[0] ), .\lo[0] (\wo0[0] ));
endmodule

module gen_d1_scalar_feedthrough_bracket_leaf (\li[0] , \lo[0] );
 input \li[0] ;
 output \lo[0] ;

 assign \lo[0]  = \li[0] ;
endmodule
