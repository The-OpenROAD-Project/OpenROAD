// TOP: top
// TECH: nangate45
// TARGETS: depth_3, scalar, gated, naming_mixed
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 3 nesting,
// scalar ports wired gated, mixed identifiers.

module top (\ti/y[1] , \to/y[1] );
 input \ti/y[1] ;
 output \to/y[1] ;

 gen_d3_scalar_gated_mixed_w2 \u_top/y[1]  (.\wi2/y[1] (\ti/y[1] ), .\wo2/y[1] (\to/y[1] ));
endmodule

module gen_d3_scalar_gated_mixed_w2 (\wi2/y[1] , \wo2/y[1] );
 input \wi2/y[1] ;
 output \wo2/y[1] ;

 gen_d3_scalar_gated_mixed_w1 \u_child/y[1]  (.\wi1/y[1] (\wi2/y[1] ), .\wo1/y[1] (\wo2/y[1] ));
endmodule

module gen_d3_scalar_gated_mixed_w1 (\wi1/y[1] , \wo1/y[1] );
 input \wi1/y[1] ;
 output \wo1/y[1] ;

 gen_d3_scalar_gated_mixed_w0 \u_child/y[1]  (.\wi0/y[1] (\wi1/y[1] ), .\wo0/y[1] (\wo1/y[1] ));
endmodule

module gen_d3_scalar_gated_mixed_w0 (\wi0/y[1] , \wo0/y[1] );
 input \wi0/y[1] ;
 output \wo0/y[1] ;

 gen_d3_scalar_gated_mixed_leaf \u_child/y[1]  (.\li/y[1] (\wi0/y[1] ), .\lo/y[1] (\wo0/y[1] ));
endmodule

module gen_d3_scalar_gated_mixed_leaf (\li/y[1] , \lo/y[1] );
 input \li/y[1] ;
 output \lo/y[1] ;

 BUF_X1 \g0/y[1]  (.A(\li/y[1] ), .Z(\lo/y[1] ));
endmodule
