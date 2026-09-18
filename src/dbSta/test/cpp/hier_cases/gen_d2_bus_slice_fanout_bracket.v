// TOP: top
// TECH: nangate45
// TARGETS: depth_2, bus_slice, fanout, naming_bracket
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 2 nesting,
// bus_slice ports wired fanout, bracket identifiers.

module top (\ti[0] , \to[0] );
 input [3:0] \ti[0] ;
 output [3:0] \to[0] ;

 gen_d2_bus_slice_fanout_bracket_w1 \u_top[0]  (.\wi1[0] (\ti[0] ), .\wo1[0] (\to[0] ));
endmodule

module gen_d2_bus_slice_fanout_bracket_w1 (\wi1[0] , \wo1[0] );
 input [3:0] \wi1[0] ;
 output [3:0] \wo1[0] ;

 gen_d2_bus_slice_fanout_bracket_w0 \u_child[0]  (.\wi0[0] ({\wi1[0] [3], \wi1[0] [2], \wi1[0] [1], \wi1[0] [0]}),
    .\wo0[0] ({\wo1[0] [3], \wo1[0] [2], \wo1[0] [1], \wo1[0] [0]}));
endmodule

module gen_d2_bus_slice_fanout_bracket_w0 (\wi0[0] , \wo0[0] );
 input [3:0] \wi0[0] ;
 output [3:0] \wo0[0] ;

 gen_d2_bus_slice_fanout_bracket_leaf \u_child[0]  (.\li[0] ({\wi0[0] [3], \wi0[0] [2], \wi0[0] [1], \wi0[0] [0]}),
    .\lo[0] ({\wo0[0] [3], \wo0[0] [2], \wo0[0] [1], \wo0[0] [0]}));
endmodule

module gen_d2_bus_slice_fanout_bracket_leaf (\li[0] , \lo[0] );
 input [3:0] \li[0] ;
 output [3:0] \lo[0] ;

 BUF_X1 \g0[0]  (.A(\li[0] [3]), .Z(\lo[0] [3]));
 BUF_X1 \g1[0]  (.A(\li[0] [3]), .Z(\lo[0] [2]));
 BUF_X1 \g2[0]  (.A(\li[0] [3]), .Z(\lo[0] [1]));
 BUF_X1 \g3[0]  (.A(\li[0] [3]), .Z(\lo[0] [0]));
endmodule
