module top (clk,
    in1,
    in2,
    in3,
    in4,
    out1,
    out2);
 input clk;
 input in1;
 input in2;
 input in3;
 input in4;
 output out1;
 output out2;


 BUF_X2 buf1 (.A(m1_out1),
    .Z(n1));
 BUF_X2 buf2 (.A(m2_out1),
    .Z(n2));
 DFF_X1 dff_out1 (.D(n1),
    .CK(clk),
    .Q(out1));
 DFF_X1 dff_out2 (.D(n2),
    .CK(clk),
    .Q(out2));
 mid1 u_mid1 (.clk(clk),
    .in1(in1),
    .in2(in2),
    .out1(m1_out1),
    .out2(m1_out2));
 mid2 u_mid2 (.clk(clk),
    .in1(in3),
    .in2(in4),
    .out1(m2_out1));
endmodule
module mid1 (clk,
    in1,
    in2,
    out1,
    out2);
 input clk;
 input in1;
 input in2;
 output out1;
 output out2;

 wire net20;
 wire net19;
 wire net18;
 wire net17;
 wire net16;
 wire net13;
 wire net12;
 wire net11;
 wire net10;
 wire net9;
 wire net8;
 wire net7;
 wire net6;
 wire net5;
 wire net4;
 wire net3;
 wire l2_out2;
 wire l2_out1;
 wire l1_out;

 DFF_X1 dff_load1 (.D(net5),
    .CK(clk));
 DFF_X1 dff_load10 (.D(net6),
    .CK(clk));
 DFF_X1 dff_load2 (.D(net7),
    .CK(clk));
 DFF_X1 dff_load3 (.D(net17),
    .CK(clk));
 DFF_X1 dff_load4 (.D(net8),
    .CK(clk));
 DFF_X1 dff_load5 (.D(net18),
    .CK(clk));
 DFF_X1 dff_load6 (.D(net9),
    .CK(clk));
 DFF_X1 dff_load7 (.D(net19),
    .CK(clk));
 DFF_X1 dff_load8 (.D(net20),
    .CK(clk));
 DFF_X1 dff_load9 (.D(net10),
    .CK(clk));
 DFF_X1 dff_out1 (.D(l1_out),
    .CK(clk),
    .Q(out1));
 DFF_X1 dff_out2 (.D(net16),
    .CK(clk),
    .Q(out2));
 leaf1 u_leaf1 (.clk(clk),
    .in1(in1),
    .out1(l1_out));
 leaf2 u_leaf2 (.net20_o(net20),
    .net19_o(net19),
    .net18_o(net18),
    .net17_o(net17),
    .net16_o(net16),
    .net13_o(net13),
    .net12_o(net12),
    .net11_o(net11),
    .net10_o(net10),
    .net9_o(net9),
    .net8_o(net8),
    .net7_o(net7),
    .net6_o(net6),
    .net5_o(net5),
    .net4_o(net4),
    .net3_o(net3),
    .clk(clk),
    .in1(in2),
    .out1(l2_out1),
    .out2(l2_out2));
endmodule
module leaf1 (clk,
    in1,
    out1);
 input clk;
 input in1;
 output out1;


 BUF_X2 buf1 (.A(n1),
    .Z(n2));
 BUF_X2 buf2 (.A(n2),
    .Z(n3));
 DFF_X1 dff1 (.D(in1),
    .CK(clk),
    .Q(n1));
 DFF_X1 dff2 (.D(n3),
    .CK(clk),
    .Q(out1));
endmodule
module leaf2 (net20_o,
    net19_o,
    net18_o,
    net17_o,
    net16_o,
    net13_o,
    net12_o,
    net11_o,
    net10_o,
    net9_o,
    net8_o,
    net7_o,
    net6_o,
    net5_o,
    net4_o,
    net3_o,
    clk,
    in1,
    out1,
    out2);
 output net20_o;
 output net19_o;
 output net18_o;
 output net17_o;
 output net16_o;
 output net13_o;
 output net12_o;
 output net11_o;
 output net10_o;
 output net9_o;
 output net8_o;
 output net7_o;
 output net6_o;
 output net5_o;
 output net4_o;
 output net3_o;
 input clk;
 input in1;
 output out1;
 output out2;

 wire net90;
 wire net89;
 wire net1;

 BUF_X4 split2 (.A(net2),
    .Z(net90));
 BUF_X4 rebuffer1 (.A(\u_mid1/l2_out1 ),
    .Z(net89));
 BUF_X2 buf1 (.A(n1),
    .Z(n2));
 BUF_X2 buf2 (.A(n1),
    .Z(n3));
 DFF_X1 dff1 (.D(in1),
    .CK(clk),
    .Q(n1));
 DFF_X1 dff2 (.D(n2),
    .CK(clk),
    .Q(net1));
 DFF_X1 dff3 (.D(n3),
    .CK(clk),
    .Q(out2));
 assign net20_o = net90;
 assign net19_o = net90;
 assign net18_o = net90;
 assign net17_o = net90;
 assign net16_o = net90;
 assign net13_o = net89;
 assign net12_o = net89;
 assign net11_o = net89;
 assign net10_o = net89;
 assign net9_o = net89;
 assign net8_o = net89;
 assign net7_o = net89;
 assign net6_o = net89;
 assign net5_o = net89;
 assign net4_o = net89;
 assign net3_o = net89;
 assign out1 = net1;
endmodule
module mid2 (clk,
    in1,
    in2,
    out1);
 input clk;
 input in1;
 input in2;
 output out1;

 wire l4_out;
 wire l3_out;

 BUF_X2 buf1 (.A(l3_out),
    .Z(n1));
 DFF_X1 dff_out1 (.D(n1),
    .CK(clk),
    .Q(out1));
 leaf3 u_leaf3 (.clk(clk),
    .in1(in1),
    .out1(l3_out));
 leaf4 u_leaf4 (.clk(clk),
    .in1(in2),
    .out1(l4_out));
endmodule
module leaf3 (clk,
    in1,
    out1);
 input clk;
 input in1;
 output out1;


 BUF_X2 buf1 (.A(n1),
    .Z(n2));
 BUF_X2 buf2 (.A(n2),
    .Z(n3));
 BUF_X2 buf3 (.A(n3),
    .Z(n4));
 BUF_X2 buf4 (.A(n4),
    .Z(n5));
 DFF_X1 dff1 (.D(in1),
    .CK(clk),
    .Q(n1));
 DFF_X1 dff2 (.D(n5),
    .CK(clk),
    .Q(out1));
endmodule
module leaf4 (clk,
    in1,
    out1);
 input clk;
 input in1;
 output out1;


 DFF_X1 dff1 (.D(in1),
    .CK(clk),
    .Q(out1));
endmodule
