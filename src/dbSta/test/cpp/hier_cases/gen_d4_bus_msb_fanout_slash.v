// TOP: top
// TECH: nangate45
// TARGETS: depth_4, bus_msb, fanout, naming_slash
// CLUE: generated sweep over hierarchy depth x signal shape x
// port wiring x identifier escaping. Depth 4 nesting,
// bus_msb ports wired fanout, slash identifiers.

module top (\ti/x , \to/x );
 input [3:0] \ti/x ;
 output [3:0] \to/x ;

 gen_d4_bus_msb_fanout_slash_w3 \u_top/x  (.\wi3/x (\ti/x ), .\wo3/x (\to/x ));
endmodule

module gen_d4_bus_msb_fanout_slash_w3 (\wi3/x , \wo3/x );
 input [3:0] \wi3/x ;
 output [3:0] \wo3/x ;

 gen_d4_bus_msb_fanout_slash_w2 \u_child/x  (.\wi2/x (\wi3/x ), .\wo2/x (\wo3/x ));
endmodule

module gen_d4_bus_msb_fanout_slash_w2 (\wi2/x , \wo2/x );
 input [3:0] \wi2/x ;
 output [3:0] \wo2/x ;

 gen_d4_bus_msb_fanout_slash_w1 \u_child/x  (.\wi1/x (\wi2/x ), .\wo1/x (\wo2/x ));
endmodule

module gen_d4_bus_msb_fanout_slash_w1 (\wi1/x , \wo1/x );
 input [3:0] \wi1/x ;
 output [3:0] \wo1/x ;

 gen_d4_bus_msb_fanout_slash_w0 \u_child/x  (.\wi0/x (\wi1/x ), .\wo0/x (\wo1/x ));
endmodule

module gen_d4_bus_msb_fanout_slash_w0 (\wi0/x , \wo0/x );
 input [3:0] \wi0/x ;
 output [3:0] \wo0/x ;

 gen_d4_bus_msb_fanout_slash_leaf \u_child/x  (.\li/x (\wi0/x ), .\lo/x (\wo0/x ));
endmodule

module gen_d4_bus_msb_fanout_slash_leaf (\li/x , \lo/x );
 input [3:0] \li/x ;
 output [3:0] \lo/x ;

 BUF_X1 \g0/x  (.A(\li/x [3]), .Z(\lo/x [3]));
 BUF_X1 \g1/x  (.A(\li/x [3]), .Z(\lo/x [2]));
 BUF_X1 \g2/x  (.A(\li/x [3]), .Z(\lo/x [1]));
 BUF_X1 \g3/x  (.A(\li/x [3]), .Z(\lo/x [0]));
endmodule
