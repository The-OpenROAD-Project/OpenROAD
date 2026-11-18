module top (clk,
    in1);
 input clk;
 input in1;

 wire net848;
 wire net5868;
 wire net5869;

 BUF_X1 input920 (.A(net5868),
    .Z(net848));
 swerv swerv (.clk(clk),
    .ifu_axi_arready(net848));
 BUF_X1 wire5940 (.A(net5869),
    .Z(net5868));
 BUF_X1 wire5941 (.A(in1),
    .Z(net5869));
endmodule
module ifu (clk,
    ifu_axi_arready);
 input clk;
 input ifu_axi_arready;


 INV_X1 _11393_ (.A(ifu_axi_arready));
 INV_X1 _11394_ (.A(ifu_axi_arready));
endmodule
module swerv (clk,
    ifu_axi_arready);
 input clk;
 input ifu_axi_arready;


 ifu ifu (.clk(clk),
    .ifu_axi_arready(ifu_axi_arready));
endmodule
