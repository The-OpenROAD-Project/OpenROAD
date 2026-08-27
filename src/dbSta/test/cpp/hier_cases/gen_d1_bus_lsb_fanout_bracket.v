// TOP: top
// TECH: nangate45
// TARGETS: depth_1, bus_lsb, fanout, naming_bracket
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// bus_lsb ports wired fanout, bracket identifiers.

module top (\ti[0] , \to[0] );
 input [0:3] \ti[0] ;
 output [0:3] \to[0] ;

 gen_d1_bus_lsb_fanout_bracket_w0 \u_top[0]  (.\wi0[0] (\ti[0] ), .\wo0[0] (\to[0] ));
endmodule

module gen_d1_bus_lsb_fanout_bracket_w0 (\wi0[0] , \wo0[0] );
 input [0:3] \wi0[0] ;
 output [0:3] \wo0[0] ;

 gen_d1_bus_lsb_fanout_bracket_leaf \u_child[0]  (.\li[0] (\wi0[0] ), .\lo[0] (\wo0[0] ));
endmodule

module gen_d1_bus_lsb_fanout_bracket_leaf (\li[0] , \lo[0] );
 input [0:3] \li[0] ;
 output [0:3] \lo[0] ;

 BUF_X1 \g0[0]  (.A(\li[0] [0]), .Z(\lo[0] [0]));
 BUF_X1 \g1[0]  (.A(\li[0] [0]), .Z(\lo[0] [1]));
 BUF_X1 \g2[0]  (.A(\li[0] [0]), .Z(\lo[0] [2]));
 BUF_X1 \g3[0]  (.A(\li[0] [0]), .Z(\lo[0] [3]));
endmodule
