// TOP: top
// TECH: nangate45
// TARGETS: depth_3, bus_slice, fanout, naming_mixed
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 3 nesting,
// bus_slice ports wired fanout, mixed identifiers.

module top (\ti/y[1] , \to/y[1] );
 input [3:0] \ti/y[1] ;
 output [3:0] \to/y[1] ;

 gen_d3_bus_slice_fanout_mixed_w2 \u_top/y[1]  (.\wi2/y[1] (\ti/y[1] ), .\wo2/y[1] (\to/y[1] ));
endmodule

module gen_d3_bus_slice_fanout_mixed_w2 (\wi2/y[1] , \wo2/y[1] );
 input [3:0] \wi2/y[1] ;
 output [3:0] \wo2/y[1] ;

 gen_d3_bus_slice_fanout_mixed_w1 \u_child/y[1]  (.\wi1/y[1] ({\wi2/y[1] [3], \wi2/y[1] [2], \wi2/y[1] [1], \wi2/y[1] [0]}),
    .\wo1/y[1] ({\wo2/y[1] [3], \wo2/y[1] [2], \wo2/y[1] [1], \wo2/y[1] [0]}));
endmodule

module gen_d3_bus_slice_fanout_mixed_w1 (\wi1/y[1] , \wo1/y[1] );
 input [3:0] \wi1/y[1] ;
 output [3:0] \wo1/y[1] ;

 gen_d3_bus_slice_fanout_mixed_w0 \u_child/y[1]  (.\wi0/y[1] ({\wi1/y[1] [3], \wi1/y[1] [2], \wi1/y[1] [1], \wi1/y[1] [0]}),
    .\wo0/y[1] ({\wo1/y[1] [3], \wo1/y[1] [2], \wo1/y[1] [1], \wo1/y[1] [0]}));
endmodule

module gen_d3_bus_slice_fanout_mixed_w0 (\wi0/y[1] , \wo0/y[1] );
 input [3:0] \wi0/y[1] ;
 output [3:0] \wo0/y[1] ;

 gen_d3_bus_slice_fanout_mixed_leaf \u_child/y[1]  (.\li/y[1] ({\wi0/y[1] [3], \wi0/y[1] [2], \wi0/y[1] [1], \wi0/y[1] [0]}),
    .\lo/y[1] ({\wo0/y[1] [3], \wo0/y[1] [2], \wo0/y[1] [1], \wo0/y[1] [0]}));
endmodule

module gen_d3_bus_slice_fanout_mixed_leaf (\li/y[1] , \lo/y[1] );
 input [3:0] \li/y[1] ;
 output [3:0] \lo/y[1] ;

 BUF_X1 \g0/y[1]  (.A(\li/y[1] [3]), .Z(\lo/y[1] [3]));
 BUF_X1 \g1/y[1]  (.A(\li/y[1] [3]), .Z(\lo/y[1] [2]));
 BUF_X1 \g2/y[1]  (.A(\li/y[1] [3]), .Z(\lo/y[1] [1]));
 BUF_X1 \g3/y[1]  (.A(\li/y[1] [3]), .Z(\lo/y[1] [0]));
endmodule
