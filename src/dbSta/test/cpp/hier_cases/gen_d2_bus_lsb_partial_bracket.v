// TOP: top
// TECH: nangate45
// TARGETS: depth_2, bus_lsb, partial, naming_bracket
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 2 nesting,
// bus_lsb ports wired partial, bracket identifiers.

module top (\ti[0] , \to[0] );
 input [0:3] \ti[0] ;
 output [0:3] \to[0] ;

 gen_d2_bus_lsb_partial_bracket_w1 \u_top[0]  (.\wi1[0] (\ti[0] ), .\wo1[0] (\to[0] ));
endmodule

module gen_d2_bus_lsb_partial_bracket_w1 (\wi1[0] , \wo1[0] );
 input [0:3] \wi1[0] ;
 output [0:3] \wo1[0] ;

 gen_d2_bus_lsb_partial_bracket_w0 \u_child[0]  (.\wi0[0] (\wi1[0] ), .\wo0[0] (\wo1[0] ));
endmodule

module gen_d2_bus_lsb_partial_bracket_w0 (\wi0[0] , \wo0[0] );
 input [0:3] \wi0[0] ;
 output [0:3] \wo0[0] ;

 gen_d2_bus_lsb_partial_bracket_leaf \u_child[0]  (.\li[0] (\wi0[0] ), .\lo[0] (\wo0[0] ));
endmodule

module gen_d2_bus_lsb_partial_bracket_leaf (\li[0] , \lo[0] );
 input [0:3] \li[0] ;
 output [0:3] \lo[0] ;

 INV_X1 \g0[0]  (.A(\li[0] [0]), .ZN(\lo[0] [0]));
 assign \lo[0] [1] = \li[0] [1];
 INV_X1 \g2[0]  (.A(\li[0] [2]), .ZN(\lo[0] [2]));
 assign \lo[0] [3] = \li[0] [3];
endmodule
