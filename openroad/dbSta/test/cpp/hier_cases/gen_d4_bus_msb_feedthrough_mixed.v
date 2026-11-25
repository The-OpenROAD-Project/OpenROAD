// TOP: top
// TECH: nangate45
// TARGETS: depth_4, bus_msb, feedthrough, naming_mixed
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 4 nesting,
// bus_msb ports wired feedthrough, mixed identifiers.

module top (\ti/y[1] , \to/y[1] );
 input [3:0] \ti/y[1] ;
 output [3:0] \to/y[1] ;

 gen_d4_bus_msb_feedthrough_mixed_w3 \u_top/y[1]  (.\wi3/y[1] (\ti/y[1] ), .\wo3/y[1] (\to/y[1] ));
endmodule

module gen_d4_bus_msb_feedthrough_mixed_w3 (\wi3/y[1] , \wo3/y[1] );
 input [3:0] \wi3/y[1] ;
 output [3:0] \wo3/y[1] ;

 gen_d4_bus_msb_feedthrough_mixed_w2 \u_child/y[1]  (.\wi2/y[1] (\wi3/y[1] ), .\wo2/y[1] (\wo3/y[1] ));
endmodule

module gen_d4_bus_msb_feedthrough_mixed_w2 (\wi2/y[1] , \wo2/y[1] );
 input [3:0] \wi2/y[1] ;
 output [3:0] \wo2/y[1] ;

 gen_d4_bus_msb_feedthrough_mixed_w1 \u_child/y[1]  (.\wi1/y[1] (\wi2/y[1] ), .\wo1/y[1] (\wo2/y[1] ));
endmodule

module gen_d4_bus_msb_feedthrough_mixed_w1 (\wi1/y[1] , \wo1/y[1] );
 input [3:0] \wi1/y[1] ;
 output [3:0] \wo1/y[1] ;

 gen_d4_bus_msb_feedthrough_mixed_w0 \u_child/y[1]  (.\wi0/y[1] (\wi1/y[1] ), .\wo0/y[1] (\wo1/y[1] ));
endmodule

module gen_d4_bus_msb_feedthrough_mixed_w0 (\wi0/y[1] , \wo0/y[1] );
 input [3:0] \wi0/y[1] ;
 output [3:0] \wo0/y[1] ;

 gen_d4_bus_msb_feedthrough_mixed_leaf \u_child/y[1]  (.\li/y[1] (\wi0/y[1] ), .\lo/y[1] (\wo0/y[1] ));
endmodule

module gen_d4_bus_msb_feedthrough_mixed_leaf (\li/y[1] , \lo/y[1] );
 input [3:0] \li/y[1] ;
 output [3:0] \lo/y[1] ;

 assign \lo/y[1] [3] = \li/y[1] [3];
 assign \lo/y[1] [2] = \li/y[1] [2];
 assign \lo/y[1] [1] = \li/y[1] [1];
 assign \lo/y[1] [0] = \li/y[1] [0];
endmodule
