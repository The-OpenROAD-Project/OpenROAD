// TOP: top
// TECH: nangate45
// TARGETS: depth_3, scalar, gated, naming_slash
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 3 nesting,
// scalar ports wired gated, slash identifiers.

module top (\ti/x , \to/x );
 input \ti/x ;
 output \to/x ;

 gen_d3_scalar_gated_slash_w2 \u_top/x  (.\wi2/x (\ti/x ), .\wo2/x (\to/x ));
endmodule

module gen_d3_scalar_gated_slash_w2 (\wi2/x , \wo2/x );
 input \wi2/x ;
 output \wo2/x ;

 gen_d3_scalar_gated_slash_w1 \u_child/x  (.\wi1/x (\wi2/x ), .\wo1/x (\wo2/x ));
endmodule

module gen_d3_scalar_gated_slash_w1 (\wi1/x , \wo1/x );
 input \wi1/x ;
 output \wo1/x ;

 gen_d3_scalar_gated_slash_w0 \u_child/x  (.\wi0/x (\wi1/x ), .\wo0/x (\wo1/x ));
endmodule

module gen_d3_scalar_gated_slash_w0 (\wi0/x , \wo0/x );
 input \wi0/x ;
 output \wo0/x ;

 gen_d3_scalar_gated_slash_leaf \u_child/x  (.\li/x (\wi0/x ), .\lo/x (\wo0/x ));
endmodule

module gen_d3_scalar_gated_slash_leaf (\li/x , \lo/x );
 input \li/x ;
 output \lo/x ;

 BUF_X1 \g0/x  (.A(\li/x ), .Z(\lo/x ));
endmodule
