// Deeper hier mimicking real designs:
//  - 3 levels: top -> mid -> leaf
//  - 4 flops per leaf, 2 leaves per mid, 1 mid -> trays size 4 land in leaves
//  - flops' Q ports drive up through leaf/mid/top hierarchy as port pins
//  - flops' D ports driven from inputs that cross hier (top -> mid -> leaf)

module leaf (clk, d0, d1, d2, d3, q0, q1, q2, q3);
  input  clk;
  input  d0, d1, d2, d3;
  output q0, q1, q2, q3;
  DFFHQNx1_ASAP7_75t_L ff0 (.D(d0), .CLK(clk), .QN(q0));
  DFFHQNx1_ASAP7_75t_L ff1 (.D(d1), .CLK(clk), .QN(q1));
  DFFHQNx1_ASAP7_75t_L ff2 (.D(d2), .CLK(clk), .QN(q2));
  DFFHQNx1_ASAP7_75t_L ff3 (.D(d3), .CLK(clk), .QN(q3));
endmodule

module mid (clk, di, qo);
  input  clk;
  input  [7:0] di;
  output [7:0] qo;

  leaf l0 (.clk(clk),
           .d0(di[0]), .d1(di[1]), .d2(di[2]), .d3(di[3]),
           .q0(qo[0]), .q1(qo[1]), .q2(qo[2]), .q3(qo[3]));
  leaf l1 (.clk(clk),
           .d0(di[4]), .d1(di[5]), .d2(di[6]), .d3(di[7]),
           .q0(qo[4]), .q1(qo[5]), .q2(qo[6]), .q3(qo[7]));
endmodule

module mbff_hier (clk1, din, dout);
  input  clk1;
  input  [7:0] din;
  output [7:0] dout;

  mid u_mid (.clk(clk1), .di(din), .qo(dout));
endmodule
