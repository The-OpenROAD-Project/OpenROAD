// TOP: top
// TECH: nangate45
// TARGETS: depth_1, bus_lsb, partial, naming_slash
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 1 nesting,
// bus_lsb ports wired partial, slash identifiers.

module top (\ti/x , \to/x );
 input [0:3] \ti/x ;
 output [0:3] \to/x ;

 gen_d1_bus_lsb_partial_slash_w0 \u_top/x  (.\wi0/x (\ti/x ), .\wo0/x (\to/x ));
endmodule

module gen_d1_bus_lsb_partial_slash_w0 (\wi0/x , \wo0/x );
 input [0:3] \wi0/x ;
 output [0:3] \wo0/x ;

 gen_d1_bus_lsb_partial_slash_leaf \u_child/x  (.\li/x (\wi0/x ), .\lo/x (\wo0/x ));
endmodule

module gen_d1_bus_lsb_partial_slash_leaf (\li/x , \lo/x );
 input [0:3] \li/x ;
 output [0:3] \lo/x ;

 INV_X1 \g0/x  (.A(\li/x [0]), .ZN(\lo/x [0]));
 assign \lo/x [1] = \li/x [1];
 INV_X1 \g2/x  (.A(\li/x [2]), .ZN(\lo/x [2]));
 assign \lo/x [3] = \li/x [3];
endmodule
