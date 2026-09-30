// TOP: top
// TECH: nangate45
// TARGETS: depth_2, bus_lsb, feedthrough, naming_slash
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 2 nesting,
// bus_lsb ports wired feedthrough, slash identifiers.

module top (\ti/x , \to/x );
 input [0:3] \ti/x ;
 output [0:3] \to/x ;

 gen_d2_bus_lsb_feedthrough_slash_w1 \u_top/x  (.\wi1/x (\ti/x ), .\wo1/x (\to/x ));
endmodule

module gen_d2_bus_lsb_feedthrough_slash_w1 (\wi1/x , \wo1/x );
 input [0:3] \wi1/x ;
 output [0:3] \wo1/x ;

 gen_d2_bus_lsb_feedthrough_slash_w0 \u_child/x  (.\wi0/x (\wi1/x ), .\wo0/x (\wo1/x ));
endmodule

module gen_d2_bus_lsb_feedthrough_slash_w0 (\wi0/x , \wo0/x );
 input [0:3] \wi0/x ;
 output [0:3] \wo0/x ;

 gen_d2_bus_lsb_feedthrough_slash_leaf \u_child/x  (.\li/x (\wi0/x ), .\lo/x (\wo0/x ));
endmodule

module gen_d2_bus_lsb_feedthrough_slash_leaf (\li/x , \lo/x );
 input [0:3] \li/x ;
 output [0:3] \lo/x ;

 assign \lo/x [0] = \li/x [0];
 assign \lo/x [1] = \li/x [1];
 assign \lo/x [2] = \li/x [2];
 assign \lo/x [3] = \li/x [3];
endmodule
