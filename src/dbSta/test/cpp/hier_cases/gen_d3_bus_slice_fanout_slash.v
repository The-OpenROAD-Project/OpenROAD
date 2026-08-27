// TOP: top
// TECH: nangate45
// TARGETS: depth_3, bus_slice, fanout, naming_slash
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 3 nesting,
// bus_slice ports wired fanout, slash identifiers.

module top (\ti/x , \to/x );
 input [3:0] \ti/x ;
 output [3:0] \to/x ;

 gen_d3_bus_slice_fanout_slash_w2 \u_top/x  (.\wi2/x (\ti/x ), .\wo2/x (\to/x ));
endmodule

module gen_d3_bus_slice_fanout_slash_w2 (\wi2/x , \wo2/x );
 input [3:0] \wi2/x ;
 output [3:0] \wo2/x ;

 gen_d3_bus_slice_fanout_slash_w1 \u_child/x  (.\wi1/x ({\wi2/x [3], \wi2/x [2], \wi2/x [1], \wi2/x [0]}),
    .\wo1/x ({\wo2/x [3], \wo2/x [2], \wo2/x [1], \wo2/x [0]}));
endmodule

module gen_d3_bus_slice_fanout_slash_w1 (\wi1/x , \wo1/x );
 input [3:0] \wi1/x ;
 output [3:0] \wo1/x ;

 gen_d3_bus_slice_fanout_slash_w0 \u_child/x  (.\wi0/x ({\wi1/x [3], \wi1/x [2], \wi1/x [1], \wi1/x [0]}),
    .\wo0/x ({\wo1/x [3], \wo1/x [2], \wo1/x [1], \wo1/x [0]}));
endmodule

module gen_d3_bus_slice_fanout_slash_w0 (\wi0/x , \wo0/x );
 input [3:0] \wi0/x ;
 output [3:0] \wo0/x ;

 gen_d3_bus_slice_fanout_slash_leaf \u_child/x  (.\li/x ({\wi0/x [3], \wi0/x [2], \wi0/x [1], \wi0/x [0]}),
    .\lo/x ({\wo0/x [3], \wo0/x [2], \wo0/x [1], \wo0/x [0]}));
endmodule

module gen_d3_bus_slice_fanout_slash_leaf (\li/x , \lo/x );
 input [3:0] \li/x ;
 output [3:0] \lo/x ;

 BUF_X1 \g0/x  (.A(\li/x [3]), .Z(\lo/x [3]));
 BUF_X1 \g1/x  (.A(\li/x [3]), .Z(\lo/x [2]));
 BUF_X1 \g2/x  (.A(\li/x [3]), .Z(\lo/x [1]));
 BUF_X1 \g3/x  (.A(\li/x [3]), .Z(\lo/x [0]));
endmodule
