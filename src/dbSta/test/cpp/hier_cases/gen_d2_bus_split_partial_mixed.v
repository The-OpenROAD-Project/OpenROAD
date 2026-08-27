// TOP: top
// TECH: nangate45
// TARGETS: depth_2, bus_split, partial, naming_mixed
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 2 nesting,
// bus_split ports wired partial, mixed identifiers.

module top (\ti/y[1] , \to/y[1] );
 input [3:0] \ti/y[1] ;
 output [3:0] \to/y[1] ;

 gen_d2_bus_split_partial_mixed_w1 \u_top/y[1]  (.\wi1/y[1] (\ti/y[1] ), .\wo1/y[1] (\to/y[1] ));
endmodule

module gen_d2_bus_split_partial_mixed_w1 (\wi1/y[1] , \wo1/y[1] );
 input [3:0] \wi1/y[1] ;
 output [3:0] \wo1/y[1] ;

 gen_d2_bus_split_partial_mixed_w0 \u_child/y[1]  (.\wi0/y[1] (\wi1/y[1] ), .\wo0/y[1] (\wo1/y[1] ));
endmodule

module gen_d2_bus_split_partial_mixed_w0 (\wi0/y[1] , \wo0/y[1] );
 input [3:0] \wi0/y[1] ;
 output [3:0] \wo0/y[1] ;

 gen_d2_bus_split_partial_mixed_leaf \u_child/y[1]  (.\li/y[1] (\wi0/y[1] ), .\lo/y[1] (\wo0/y[1] ));
endmodule

module gen_d2_bus_split_partial_mixed_leaf (\li/y[1] , \lo/y[1] );
 input [3:0] \li/y[1] ;
 output [3:0] \lo/y[1] ;

 INV_X1 \g0/y[1]  (.A(\li/y[1] [3]), .ZN(\lo/y[1] [3]));
 assign \lo/y[1] [2] = \li/y[1] [2];
 INV_X1 \g2/y[1]  (.A(\li/y[1] [1]), .ZN(\lo/y[1] [1]));
 assign \lo/y[1] [0] = \li/y[1] [0];
endmodule
