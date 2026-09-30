// TOP: top
// TECH: nangate45
// TARGETS: depth_2, bus_lsb, fanout, naming_slash
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 2 nesting,
// bus_lsb ports wired fanout, slash identifiers.

module top (\ti/x , \to/x );
 input [0:3] \ti/x ;
 output [0:3] \to/x ;

 gen_d2_bus_lsb_fanout_slash_w1 \u_top/x  (.\wi1/x (\ti/x ), .\wo1/x (\to/x ));
endmodule

module gen_d2_bus_lsb_fanout_slash_w1 (\wi1/x , \wo1/x );
 input [0:3] \wi1/x ;
 output [0:3] \wo1/x ;

 gen_d2_bus_lsb_fanout_slash_w0 \u_child/x  (.\wi0/x (\wi1/x ), .\wo0/x (\wo1/x ));
endmodule

module gen_d2_bus_lsb_fanout_slash_w0 (\wi0/x , \wo0/x );
 input [0:3] \wi0/x ;
 output [0:3] \wo0/x ;

 gen_d2_bus_lsb_fanout_slash_leaf \u_child/x  (.\li/x (\wi0/x ), .\lo/x (\wo0/x ));
endmodule

module gen_d2_bus_lsb_fanout_slash_leaf (\li/x , \lo/x );
 input [0:3] \li/x ;
 output [0:3] \lo/x ;

 BUF_X1 \g0/x  (.A(\li/x [0]), .Z(\lo/x [0]));
 BUF_X1 \g1/x  (.A(\li/x [0]), .Z(\lo/x [1]));
 BUF_X1 \g2/x  (.A(\li/x [0]), .Z(\lo/x [2]));
 BUF_X1 \g3/x  (.A(\li/x [0]), .Z(\lo/x [3]));
endmodule
