module hi_fanout (a,
    clk1,
    in);
 output a;
 input clk1;
 input in;


 DFF_X1 drvr (.D(in),
    .CK(clk1),
    .Q(a));
 hi_fanout_child hi_fanout_inst1 (.a(a),
    .clk1(clk1));
 hi_fanout_child_hi_fanout_inst2 hi_fanout_inst2 (.a(a),
    .clk1(clk1));
endmodule
module hi_fanout_child (a,
    clk1);
 input a;
 input clk1;


 DFF_X1 load0 (.D(a),
    .CK(clk1));
 DFF_X1 load1 (.D(a),
    .CK(clk1));
 DFF_X1 load10 (.D(a),
    .CK(clk1));
 DFF_X1 load11 (.D(a),
    .CK(clk1));
 DFF_X1 load12 (.D(a),
    .CK(clk1));
 DFF_X1 load13 (.D(a),
    .CK(clk1));
 DFF_X1 load14 (.D(a),
    .CK(clk1));
 DFF_X1 load15 (.D(a),
    .CK(clk1));
 DFF_X1 load16 (.D(a),
    .CK(clk1));
 DFF_X1 load17 (.D(a),
    .CK(clk1));
 DFF_X1 load18 (.D(a),
    .CK(clk1));
 DFF_X1 load19 (.D(a),
    .CK(clk1));
 DFF_X1 load2 (.D(a),
    .CK(clk1));
 DFF_X1 load20 (.D(a),
    .CK(clk1));
 DFF_X1 load21 (.D(a),
    .CK(clk1));
 DFF_X1 load22 (.D(a),
    .CK(clk1));
 DFF_X1 load23 (.D(a),
    .CK(clk1));
 DFF_X1 load24 (.D(a),
    .CK(clk1));
 DFF_X1 load25 (.D(a),
    .CK(clk1));
 DFF_X1 load26 (.D(a),
    .CK(clk1));
 DFF_X1 load27 (.D(a),
    .CK(clk1));
 DFF_X1 load28 (.D(a),
    .CK(clk1));
 DFF_X1 load29 (.D(a),
    .CK(clk1));
 DFF_X1 load3 (.D(a),
    .CK(clk1));
 DFF_X1 load30 (.D(a),
    .CK(clk1));
 DFF_X1 load31 (.D(a),
    .CK(clk1));
 DFF_X1 load32 (.D(a),
    .CK(clk1));
 DFF_X1 load33 (.D(a),
    .CK(clk1));
 DFF_X1 load34 (.D(a),
    .CK(clk1));
 DFF_X1 load4 (.D(a),
    .CK(clk1));
 DFF_X1 load5 (.D(a),
    .CK(clk1));
 DFF_X1 load6 (.D(a),
    .CK(clk1));
 DFF_X1 load7 (.D(a),
    .CK(clk1));
 DFF_X1 load8 (.D(a),
    .CK(clk1));
 DFF_X1 load9 (.D(a),
    .CK(clk1));
endmodule
module hi_fanout_child_hi_fanout_inst2 (a,
    clk1);
 input a;
 input clk1;


 DFF_X1 load0 (.D(a),
    .CK(clk1));
 DFF_X1 load1 (.D(a),
    .CK(clk1));
 DFF_X1 load10 (.D(a),
    .CK(clk1));
 DFF_X1 load11 (.D(a),
    .CK(clk1));
 DFF_X1 load12 (.D(a),
    .CK(clk1));
 DFF_X1 load13 (.D(a),
    .CK(clk1));
 DFF_X1 load14 (.D(a),
    .CK(clk1));
 DFF_X1 load15 (.D(a),
    .CK(clk1));
 DFF_X1 load16 (.D(a),
    .CK(clk1));
 DFF_X1 load17 (.D(a),
    .CK(clk1));
 DFF_X1 load18 (.D(a),
    .CK(clk1));
 DFF_X1 load19 (.D(a),
    .CK(clk1));
 DFF_X1 load2 (.D(a),
    .CK(clk1));
 DFF_X1 load20 (.D(a),
    .CK(clk1));
 DFF_X1 load21 (.D(a),
    .CK(clk1));
 DFF_X1 load22 (.D(a),
    .CK(clk1));
 DFF_X1 load23 (.D(a),
    .CK(clk1));
 DFF_X1 load24 (.D(a),
    .CK(clk1));
 DFF_X1 load25 (.D(a),
    .CK(clk1));
 DFF_X1 load26 (.D(a),
    .CK(clk1));
 DFF_X1 load27 (.D(a),
    .CK(clk1));
 DFF_X1 load28 (.D(a),
    .CK(clk1));
 DFF_X1 load29 (.D(a),
    .CK(clk1));
 DFF_X1 load3 (.D(a),
    .CK(clk1));
 DFF_X1 load30 (.D(a),
    .CK(clk1));
 DFF_X1 load31 (.D(a),
    .CK(clk1));
 DFF_X1 load32 (.D(a),
    .CK(clk1));
 DFF_X1 load33 (.D(a),
    .CK(clk1));
 DFF_X1 load34 (.D(a),
    .CK(clk1));
 DFF_X1 load4 (.D(a),
    .CK(clk1));
 DFF_X1 load5 (.D(a),
    .CK(clk1));
 DFF_X1 load6 (.D(a),
    .CK(clk1));
 DFF_X1 load7 (.D(a),
    .CK(clk1));
 DFF_X1 load8 (.D(a),
    .CK(clk1));
 DFF_X1 load9 (.D(a),
    .CK(clk1));
endmodule
