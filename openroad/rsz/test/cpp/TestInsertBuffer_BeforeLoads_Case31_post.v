module top (clk,
    in1);
 input clk;
 input in1;

 wire net;
 wire net848;
 wire net5868;

 BUF_X1 input920 (.A(net5868),
    .Z(net848));
 swerv swerv (.net(net),
    .clk(clk),
    .ifu_axi_arready(net848));
 BUF_X1 wire5940 (.A(net),
    .Z(net5868));
 BUF_X1 wire5941 (.A(in1),
    .Z(net));
endmodule
module ifu (net1,
    clk,
    ifu_axi_arready);
 input net1;
 input clk;
 input ifu_axi_arready;

 wire net;

 INV_X1 _11393_ (.A(net));
 INV_X1 _11394_ (.A(net));
 BUF_X4 new_buf (.A(net1),
    .Z(net));
endmodule
module swerv (net,
    clk,
    ifu_axi_arready);
 input net;
 input clk;
 input ifu_axi_arready;


 ifu ifu (.net1(net),
    .clk(clk),
    .ifu_axi_arready(ifu_axi_arready));
endmodule
