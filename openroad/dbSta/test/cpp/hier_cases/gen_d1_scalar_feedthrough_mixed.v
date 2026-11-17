// TOP: top
// TECH: nangate45
// TARGETS: depth_1, scalar, feedthrough, naming_mixed
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// scalar ports wired feedthrough, mixed identifiers.

module top (\ti/y[1] , \to/y[1] );
 input \ti/y[1] ;
 output \to/y[1] ;

 gen_d1_scalar_feedthrough_mixed_w0 \u_top/y[1]  (.\wi0/y[1] (\ti/y[1] ), .\wo0/y[1] (\to/y[1] ));
endmodule

module gen_d1_scalar_feedthrough_mixed_w0 (\wi0/y[1] , \wo0/y[1] );
 input \wi0/y[1] ;
 output \wo0/y[1] ;

 gen_d1_scalar_feedthrough_mixed_leaf \u_child/y[1]  (.\li/y[1] (\wi0/y[1] ), .\lo/y[1] (\wo0/y[1] ));
endmodule

module gen_d1_scalar_feedthrough_mixed_leaf (\li/y[1] , \lo/y[1] );
 input \li/y[1] ;
 output \lo/y[1] ;

 assign \lo/y[1]  = \li/y[1] ;
endmodule
