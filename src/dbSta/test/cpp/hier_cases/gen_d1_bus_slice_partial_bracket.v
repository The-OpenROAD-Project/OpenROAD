// TOP: top
// TECH: nangate45
// TARGETS: depth_1, bus_slice, partial, naming_bracket
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// bus_slice ports wired partial, bracket identifiers.

module top (\ti[0] , \to[0] );
 input [3:0] \ti[0] ;
 output [3:0] \to[0] ;

 gen_d1_bus_slice_partial_bracket_w0 \u_top[0]  (.\wi0[0] (\ti[0] ), .\wo0[0] (\to[0] ));
endmodule

module gen_d1_bus_slice_partial_bracket_w0 (\wi0[0] , \wo0[0] );
 input [3:0] \wi0[0] ;
 output [3:0] \wo0[0] ;

 gen_d1_bus_slice_partial_bracket_leaf \u_child[0]  (.\li[0] ({\wi0[0] [3], \wi0[0] [2], \wi0[0] [1], \wi0[0] [0]}),
    .\lo[0] ({\wo0[0] [3], \wo0[0] [2], \wo0[0] [1], \wo0[0] [0]}));
endmodule

module gen_d1_bus_slice_partial_bracket_leaf (\li[0] , \lo[0] );
 input [3:0] \li[0] ;
 output [3:0] \lo[0] ;

 INV_X1 \g0[0]  (.A(\li[0] [3]), .ZN(\lo[0] [3]));
 assign \lo[0] [2] = \li[0] [2];
 INV_X1 \g2[0]  (.A(\li[0] [1]), .ZN(\lo[0] [1]));
 assign \lo[0] [0] = \li[0] [0];
endmodule
