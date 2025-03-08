
module hi_fanout (clk1);
   input clk1;
   hi_fanout_child hi_fanout_inst(clk1);
endmodule

module hi_fanout_child (clk1);
   input clk1;
   wire  net0;

 DFF_X1 drvr (.CK(clk1),
    .Q(net0));
 DFF_X1 load0 (.D(net0),
    .CK(clk1));
 DFF_X1 load1 (.D(net0),
    .CK(clk1));
 DFF_X1 load2 (.D(net0),
    .CK(clk1));
 DFF_X1 load3 (.D(net0),
    .CK(clk1));
 DFF_X1 load4 (.D(net0),
    .CK(clk1));
 DFF_X1 load5 (.D(net0),
    .CK(clk1));
 DFF_X1 load6 (.D(net0),
    .CK(clk1));
 DFF_X1 load7 (.D(net0),
    .CK(clk1));
 DFF_X1 load8 (.D(net0),
    .CK(clk1));
 DFF_X1 load9 (.D(net0),
    .CK(clk1));
 DFF_X1 load10 (.D(net0),
    .CK(clk1));
 DFF_X1 load11 (.D(net0),
    .CK(clk1));
 DFF_X1 load12 (.D(net0),
    .CK(clk1));
 DFF_X1 load13 (.D(net0),
    .CK(clk1));
 DFF_X1 load14 (.D(net0),
    .CK(clk1));
 DFF_X1 load15 (.D(net0),
    .CK(clk1));
 DFF_X1 load16 (.D(net0),
    .CK(clk1));
 DFF_X1 load17 (.D(net0),
    .CK(clk1));
 DFF_X1 load18 (.D(net0),
    .CK(clk1));
 DFF_X1 load19 (.D(net0),
    .CK(clk1));
 DFF_X1 load20 (.D(net0),
    .CK(clk1));
 DFF_X1 load21 (.D(net0),
    .CK(clk1));
 DFF_X1 load22 (.D(net0),
    .CK(clk1));
 DFF_X1 load23 (.D(net0),
    .CK(clk1));
 DFF_X1 load24 (.D(net0),
    .CK(clk1));
 DFF_X1 load25 (.D(net0),
    .CK(clk1));
 DFF_X1 load26 (.D(net0),
    .CK(clk1));
 DFF_X1 load27 (.D(net0),
    .CK(clk1));
 DFF_X1 load28 (.D(net0),
    .CK(clk1));
 DFF_X1 load29 (.D(net0),
    .CK(clk1));
 DFF_X1 load30 (.D(net0),
    .CK(clk1));
 DFF_X1 load31 (.D(net0),
    .CK(clk1));
 DFF_X1 load32 (.D(net0),
    .CK(clk1));
 DFF_X1 load33 (.D(net0),
    .CK(clk1));
 DFF_X1 load34 (.D(net0),
    .CK(clk1));
   
endmodule
