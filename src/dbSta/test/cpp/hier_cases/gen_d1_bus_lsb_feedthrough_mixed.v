// TOP: top
// TECH: nangate45
// TARGETS: depth_1, bus_lsb, feedthrough, naming_mixed
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// bus_lsb ports wired feedthrough, mixed identifiers.

module top (\ti/y[1] , \to/y[1] );
 input [0:3] \ti/y[1] ;
 output [0:3] \to/y[1] ;

 gen_d1_bus_lsb_feedthrough_mixed_w0 \u_top/y[1]  (.\wi0/y[1] (\ti/y[1] ), .\wo0/y[1] (\to/y[1] ));
endmodule

module gen_d1_bus_lsb_feedthrough_mixed_w0 (\wi0/y[1] , \wo0/y[1] );
 input [0:3] \wi0/y[1] ;
 output [0:3] \wo0/y[1] ;

 gen_d1_bus_lsb_feedthrough_mixed_leaf \u_child/y[1]  (.\li/y[1] (\wi0/y[1] ), .\lo/y[1] (\wo0/y[1] ));
endmodule

module gen_d1_bus_lsb_feedthrough_mixed_leaf (\li/y[1] , \lo/y[1] );
 input [0:3] \li/y[1] ;
 output [0:3] \lo/y[1] ;

 assign \lo/y[1] [0] = \li/y[1] [0];
 assign \lo/y[1] [1] = \li/y[1] [1];
 assign \lo/y[1] [2] = \li/y[1] [2];
 assign \lo/y[1] [3] = \li/y[1] [3];
endmodule
