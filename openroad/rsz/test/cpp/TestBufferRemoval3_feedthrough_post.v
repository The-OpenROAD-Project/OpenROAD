module top (clk,
    data_o,
    in1,
    out1);
 input clk;
 output data_o;
 input in1;
 output out1;

 wire data_internal;

 DFF_X1 r1 (.D(in1),
    .CK(clk),
    .Q(data_internal));
 child_mod u_child (.data_i(data_internal),
    .data_o(data_o),
    .in1(in1),
    .out1(out1));
endmodule
module child_mod (data_i,
    data_o,
    in1,
    out1);
 input data_i;
 output data_o;
 input in1;
 output out1;


 assign out1 = in1;
 assign data_o = data_i;
endmodule
