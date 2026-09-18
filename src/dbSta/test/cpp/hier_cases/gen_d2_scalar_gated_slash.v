// TOP: top
// TECH: nangate45
// TARGETS: depth_2, scalar, gated, naming_slash
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 2 nesting,
// scalar ports wired gated, slash identifiers.

module top (\ti/x , \to/x );
 input \ti/x ;
 output \to/x ;

 gen_d2_scalar_gated_slash_w1 \u_top/x  (.\wi1/x (\ti/x ), .\wo1/x (\to/x ));
endmodule

module gen_d2_scalar_gated_slash_w1 (\wi1/x , \wo1/x );
 input \wi1/x ;
 output \wo1/x ;

 gen_d2_scalar_gated_slash_w0 \u_child/x  (.\wi0/x (\wi1/x ), .\wo0/x (\wo1/x ));
endmodule

module gen_d2_scalar_gated_slash_w0 (\wi0/x , \wo0/x );
 input \wi0/x ;
 output \wo0/x ;

 gen_d2_scalar_gated_slash_leaf \u_child/x  (.\li/x (\wi0/x ), .\lo/x (\wo0/x ));
endmodule

module gen_d2_scalar_gated_slash_leaf (\li/x , \lo/x );
 input \li/x ;
 output \lo/x ;

 BUF_X1 \g0/x  (.A(\li/x ), .Z(\lo/x ));
endmodule
