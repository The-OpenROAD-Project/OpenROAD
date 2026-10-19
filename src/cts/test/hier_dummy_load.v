// Clock tree inside a sub-module: the clock root buffer and all flops are in
// module sub, so CTS builds the tree (and its dummy loads) in sub.
module sub (clk_in, d, q);
  input clk_in;
  input d;
  output q;
  wire gclk;
  wire [39:0] r;

  BUF_X4 u_root (.A(clk_in), .Z(gclk));
  DFF_X1 ff_0 (.CK(gclk), .D(d), .Q(r[0]));
  DFF_X1 ff_1 (.CK(gclk), .D(r[0]), .Q(r[1]));
  DFF_X1 ff_2 (.CK(gclk), .D(r[1]), .Q(r[2]));
  DFF_X1 ff_3 (.CK(gclk), .D(r[2]), .Q(r[3]));
  DFF_X1 ff_4 (.CK(gclk), .D(r[3]), .Q(r[4]));
  DFF_X1 ff_5 (.CK(gclk), .D(r[4]), .Q(r[5]));
  DFF_X1 ff_6 (.CK(gclk), .D(r[5]), .Q(r[6]));
  DFF_X1 ff_7 (.CK(gclk), .D(r[6]), .Q(r[7]));
  DFF_X1 ff_8 (.CK(gclk), .D(r[7]), .Q(r[8]));
  DFF_X1 ff_9 (.CK(gclk), .D(r[8]), .Q(r[9]));
  DFF_X1 ff_10 (.CK(gclk), .D(r[9]), .Q(r[10]));
  DFF_X1 ff_11 (.CK(gclk), .D(r[10]), .Q(r[11]));
  DFF_X1 ff_12 (.CK(gclk), .D(r[11]), .Q(r[12]));
  DFF_X1 ff_13 (.CK(gclk), .D(r[12]), .Q(r[13]));
  DFF_X1 ff_14 (.CK(gclk), .D(r[13]), .Q(r[14]));
  DFF_X1 ff_15 (.CK(gclk), .D(r[14]), .Q(r[15]));
  DFF_X1 ff_16 (.CK(gclk), .D(r[15]), .Q(r[16]));
  DFF_X1 ff_17 (.CK(gclk), .D(r[16]), .Q(r[17]));
  DFF_X1 ff_18 (.CK(gclk), .D(r[17]), .Q(r[18]));
  DFF_X1 ff_19 (.CK(gclk), .D(r[18]), .Q(r[19]));
  DFF_X1 ff_20 (.CK(gclk), .D(r[19]), .Q(r[20]));
  DFF_X1 ff_21 (.CK(gclk), .D(r[20]), .Q(r[21]));
  DFF_X1 ff_22 (.CK(gclk), .D(r[21]), .Q(r[22]));
  DFF_X1 ff_23 (.CK(gclk), .D(r[22]), .Q(r[23]));
  DFF_X1 ff_24 (.CK(gclk), .D(r[23]), .Q(r[24]));
  DFF_X1 ff_25 (.CK(gclk), .D(r[24]), .Q(r[25]));
  DFF_X1 ff_26 (.CK(gclk), .D(r[25]), .Q(r[26]));
  DFF_X1 ff_27 (.CK(gclk), .D(r[26]), .Q(r[27]));
  DFF_X1 ff_28 (.CK(gclk), .D(r[27]), .Q(r[28]));
  DFF_X1 ff_29 (.CK(gclk), .D(r[28]), .Q(r[29]));
  DFF_X1 ff_30 (.CK(gclk), .D(r[29]), .Q(r[30]));
  DFF_X1 ff_31 (.CK(gclk), .D(r[30]), .Q(r[31]));
  DFF_X1 ff_32 (.CK(gclk), .D(r[31]), .Q(r[32]));
  DFF_X1 ff_33 (.CK(gclk), .D(r[32]), .Q(r[33]));
  DFF_X1 ff_34 (.CK(gclk), .D(r[33]), .Q(r[34]));
  DFF_X1 ff_35 (.CK(gclk), .D(r[34]), .Q(r[35]));
  DFF_X1 ff_36 (.CK(gclk), .D(r[35]), .Q(r[36]));
  DFF_X1 ff_37 (.CK(gclk), .D(r[36]), .Q(r[37]));
  DFF_X1 ff_38 (.CK(gclk), .D(r[37]), .Q(r[38]));
  DFF_X1 ff_39 (.CK(gclk), .D(r[38]), .Q(r[39]));
  assign q = r[39];
endmodule

module top (clk, d, q);
  input clk;
  input d;
  output q;

  sub u_sub (.clk_in(clk), .d(d), .q(q));
endmodule
