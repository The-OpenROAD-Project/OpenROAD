// TOP: top
// TECH: nangate45
// TARGETS: depth_2, bus_split, gated, naming_slash
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 2 nesting,
// bus_split ports wired gated, slash identifiers.

module top (\ti/x , \to/x );
 input [3:0] \ti/x ;
 output [3:0] \to/x ;

 gen_d2_bus_split_gated_slash_w1 \u_top/x  (.\wi1/x (\ti/x ), .\wo1/x (\to/x ));
endmodule

module gen_d2_bus_split_gated_slash_w1 (\wi1/x , \wo1/x );
 input [3:0] \wi1/x ;
 output [3:0] \wo1/x ;

 gen_d2_bus_split_gated_slash_w0 \u_child/x  (.\wi0/x (\wi1/x ), .\wo0/x (\wo1/x ));
endmodule

module gen_d2_bus_split_gated_slash_w0 (\wi0/x , \wo0/x );
 input [3:0] \wi0/x ;
 output [3:0] \wo0/x ;

 gen_d2_bus_split_gated_slash_leaf \u_child/x  (.\li/x (\wi0/x ), .\lo/x (\wo0/x ));
endmodule

module gen_d2_bus_split_gated_slash_leaf (\li/x , \lo/x );
 input [3:0] \li/x ;
 output [3:0] \lo/x ;

 BUF_X1 \g0/x  (.A(\li/x [3]), .Z(\lo/x [3]));
 BUF_X1 \g1/x  (.A(\li/x [2]), .Z(\lo/x [2]));
 BUF_X1 \g2/x  (.A(\li/x [1]), .Z(\lo/x [1]));
 BUF_X1 \g3/x  (.A(\li/x [0]), .Z(\lo/x [0]));
endmodule
