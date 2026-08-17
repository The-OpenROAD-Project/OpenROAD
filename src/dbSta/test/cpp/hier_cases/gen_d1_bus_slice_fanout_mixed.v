// TOP: top
// TECH: nangate45
// TARGETS: depth_1, bus_slice, fanout, naming_mixed
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// bus_slice ports wired fanout, mixed identifiers.

module top (\ti/y[1] , \to/y[1] );
 input [3:0] \ti/y[1] ;
 output [3:0] \to/y[1] ;

 gen_d1_bus_slice_fanout_mixed_w0 \u_top/y[1]  (.\wi0/y[1] (\ti/y[1] ), .\wo0/y[1] (\to/y[1] ));
endmodule

module gen_d1_bus_slice_fanout_mixed_w0 (\wi0/y[1] , \wo0/y[1] );
 input [3:0] \wi0/y[1] ;
 output [3:0] \wo0/y[1] ;

 gen_d1_bus_slice_fanout_mixed_leaf \u_child/y[1]  (.\li/y[1] ({\wi0/y[1] [3], \wi0/y[1] [2], \wi0/y[1] [1], \wi0/y[1] [0]}),
    .\lo/y[1] ({\wo0/y[1] [3], \wo0/y[1] [2], \wo0/y[1] [1], \wo0/y[1] [0]}));
endmodule

module gen_d1_bus_slice_fanout_mixed_leaf (\li/y[1] , \lo/y[1] );
 input [3:0] \li/y[1] ;
 output [3:0] \lo/y[1] ;

 BUF_X1 \g0/y[1]  (.A(\li/y[1] [3]), .Z(\lo/y[1] [3]));
 BUF_X1 \g1/y[1]  (.A(\li/y[1] [3]), .Z(\lo/y[1] [2]));
 BUF_X1 \g2/y[1]  (.A(\li/y[1] [3]), .Z(\lo/y[1] [1]));
 BUF_X1 \g3/y[1]  (.A(\li/y[1] [3]), .Z(\lo/y[1] [0]));
endmodule
