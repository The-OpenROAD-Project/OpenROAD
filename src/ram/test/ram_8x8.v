module RAM8x8 (clock,
    D,
    Q,
    bit,
    write_enable);
 input clock;
 input [7:0] D;
 output [7:0] Q;
 input [2:0] bit;
 input [0:0] write_enable;

 wire \D_nets[0].net ;
 wire \D_nets[1].net ;
 wire \D_nets[2].net ;
 wire \D_nets[3].net ;
 wire \D_nets[4].net ;
 wire \D_nets[5].net ;
 wire \D_nets[6].net ;
 wire \D_nets[7].net ;
 wire \storage_0_0.decoder0 ;
 wire \storage_0_0.select0_b ;
 wire \storage_0_0.clock_b ;
 wire \storage_0_0.gclock ;
 wire \storage_0_0.we0 ;
 wire \storage_0_0.bit0.storage ;
 wire \storage_0_0.bit1.storage ;
 wire \storage_0_0.bit2.storage ;
 wire \storage_0_0.bit3.storage ;
 wire \storage_0_0.bit4.storage ;
 wire \storage_0_0.bit5.storage ;
 wire \storage_0_0.bit6.storage ;
 wire \storage_0_0.bit7.storage ;
 wire \decoder0.layer_in0 ;
 wire \decoder0.layer_in1 ;
 wire \storage_1_0.decoder0 ;
 wire \storage_1_0.select0_b ;
 wire \storage_1_0.clock_b ;
 wire \storage_1_0.gclock ;
 wire \storage_1_0.we0 ;
 wire \storage_1_0.bit0.storage ;
 wire \storage_1_0.bit1.storage ;
 wire \storage_1_0.bit2.storage ;
 wire \storage_1_0.bit3.storage ;
 wire \storage_1_0.bit4.storage ;
 wire \storage_1_0.bit5.storage ;
 wire \storage_1_0.bit6.storage ;
 wire \storage_1_0.bit7.storage ;
 wire \decoder1.layer_in0 ;
 wire \decoder1.layer_in1 ;
 wire \storage_2_0.decoder0 ;
 wire \storage_2_0.select0_b ;
 wire \storage_2_0.clock_b ;
 wire \storage_2_0.gclock ;
 wire \storage_2_0.we0 ;
 wire \storage_2_0.bit0.storage ;
 wire \storage_2_0.bit1.storage ;
 wire \storage_2_0.bit2.storage ;
 wire \storage_2_0.bit3.storage ;
 wire \storage_2_0.bit4.storage ;
 wire \storage_2_0.bit5.storage ;
 wire \storage_2_0.bit6.storage ;
 wire \storage_2_0.bit7.storage ;
 wire \decoder2.layer_in0 ;
 wire \decoder2.layer_in1 ;
 wire \storage_3_0.decoder0 ;
 wire \storage_3_0.select0_b ;
 wire \storage_3_0.clock_b ;
 wire \storage_3_0.gclock ;
 wire \storage_3_0.we0 ;
 wire \storage_3_0.bit0.storage ;
 wire \storage_3_0.bit1.storage ;
 wire \storage_3_0.bit2.storage ;
 wire \storage_3_0.bit3.storage ;
 wire \storage_3_0.bit4.storage ;
 wire \storage_3_0.bit5.storage ;
 wire \storage_3_0.bit6.storage ;
 wire \storage_3_0.bit7.storage ;
 wire \decoder3.layer_in0 ;
 wire \decoder3.layer_in1 ;
 wire \storage_4_0.decoder0 ;
 wire \storage_4_0.select0_b ;
 wire \storage_4_0.clock_b ;
 wire \storage_4_0.gclock ;
 wire \storage_4_0.we0 ;
 wire \storage_4_0.bit0.storage ;
 wire \storage_4_0.bit1.storage ;
 wire \storage_4_0.bit2.storage ;
 wire \storage_4_0.bit3.storage ;
 wire \storage_4_0.bit4.storage ;
 wire \storage_4_0.bit5.storage ;
 wire \storage_4_0.bit6.storage ;
 wire \storage_4_0.bit7.storage ;
 wire \decoder4.layer_in0 ;
 wire \decoder4.layer_in1 ;
 wire \storage_5_0.decoder0 ;
 wire \storage_5_0.select0_b ;
 wire \storage_5_0.clock_b ;
 wire \storage_5_0.gclock ;
 wire \storage_5_0.we0 ;
 wire \storage_5_0.bit0.storage ;
 wire \storage_5_0.bit1.storage ;
 wire \storage_5_0.bit2.storage ;
 wire \storage_5_0.bit3.storage ;
 wire \storage_5_0.bit4.storage ;
 wire \storage_5_0.bit5.storage ;
 wire \storage_5_0.bit6.storage ;
 wire \storage_5_0.bit7.storage ;
 wire \decoder5.layer_in0 ;
 wire \decoder5.layer_in1 ;
 wire \storage_6_0.decoder0 ;
 wire \storage_6_0.select0_b ;
 wire \storage_6_0.clock_b ;
 wire \storage_6_0.gclock ;
 wire \storage_6_0.we0 ;
 wire \storage_6_0.bit0.storage ;
 wire \storage_6_0.bit1.storage ;
 wire \storage_6_0.bit2.storage ;
 wire \storage_6_0.bit3.storage ;
 wire \storage_6_0.bit4.storage ;
 wire \storage_6_0.bit5.storage ;
 wire \storage_6_0.bit6.storage ;
 wire \storage_6_0.bit7.storage ;
 wire \decoder6.layer_in0 ;
 wire \decoder6.layer_in1 ;
 wire \storage_7_0.decoder0 ;
 wire \storage_7_0.select0_b ;
 wire \storage_7_0.clock_b ;
 wire \storage_7_0.gclock ;
 wire \storage_7_0.we0 ;
 wire \storage_7_0.bit0.storage ;
 wire \storage_7_0.bit1.storage ;
 wire \storage_7_0.bit2.storage ;
 wire \storage_7_0.bit3.storage ;
 wire \storage_7_0.bit4.storage ;
 wire \storage_7_0.bit5.storage ;
 wire \storage_7_0.bit6.storage ;
 wire \storage_7_0.bit7.storage ;
 wire \decoder7.layer_in0 ;
 wire \decoder7.layer_in1 ;
 wire [2:0] \inv.bit ;

 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_0_0.bit0.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit0.obuf0  (.A(\storage_0_0.bit0.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_0_0.bit1.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit1.obuf0  (.A(\storage_0_0.bit1.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_0_0.bit2.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit2.obuf0  (.A(\storage_0_0.bit2.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_0_0.bit3.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit3.obuf0  (.A(\storage_0_0.bit3.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_0_0.bit4.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit4.obuf0  (.A(\storage_0_0.bit4.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_0_0.bit5.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit5.obuf0  (.A(\storage_0_0.bit5.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_0_0.bit6.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit6.obuf0  (.A(\storage_0_0.bit6.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_0_0.bit7.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit7.obuf0  (.A(\storage_0_0.bit7.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_0_0.cg  (.GATE(\storage_0_0.we0 ),
    .GCLK(\storage_0_0.gclock ),
    .CLK(\storage_0_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \storage_0_0.gcand  (.A(\storage_0_0.decoder0 ),
    .B(write_enable[0]),
    .X(\storage_0_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_0_0.select_inv_0  (.A(\storage_0_0.decoder0 ),
    .Y(\storage_0_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_0_0.clock_inv  (.A(clock),
    .Y(\storage_0_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \decoder0.and_layer0  (.A(\inv.bit [0]),
    .B(\decoder0.layer_in0 ),
    .X(\storage_0_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder0.and_layer1  (.A(\inv.bit [1]),
    .B(\inv.bit [2]),
    .X(\decoder0.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_1_0.bit0.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit0.obuf0  (.A(\storage_1_0.bit0.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_1_0.bit1.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit1.obuf0  (.A(\storage_1_0.bit1.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_1_0.bit2.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit2.obuf0  (.A(\storage_1_0.bit2.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_1_0.bit3.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit3.obuf0  (.A(\storage_1_0.bit3.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_1_0.bit4.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit4.obuf0  (.A(\storage_1_0.bit4.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_1_0.bit5.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit5.obuf0  (.A(\storage_1_0.bit5.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_1_0.bit6.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit6.obuf0  (.A(\storage_1_0.bit6.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_1_0.bit7.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit7.obuf0  (.A(\storage_1_0.bit7.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_1_0.cg  (.GATE(\storage_1_0.we0 ),
    .GCLK(\storage_1_0.gclock ),
    .CLK(\storage_1_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \storage_1_0.gcand  (.A(\storage_1_0.decoder0 ),
    .B(write_enable[0]),
    .X(\storage_1_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_1_0.select_inv_0  (.A(\storage_1_0.decoder0 ),
    .Y(\storage_1_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_1_0.clock_inv  (.A(clock),
    .Y(\storage_1_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \decoder1.and_layer0  (.A(\inv.bit [0]),
    .B(\decoder1.layer_in0 ),
    .X(\storage_1_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder1.and_layer1  (.A(\inv.bit [1]),
    .B(bit[2]),
    .X(\decoder1.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_2_0.bit0.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit0.obuf0  (.A(\storage_2_0.bit0.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_2_0.bit1.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit1.obuf0  (.A(\storage_2_0.bit1.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_2_0.bit2.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit2.obuf0  (.A(\storage_2_0.bit2.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_2_0.bit3.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit3.obuf0  (.A(\storage_2_0.bit3.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_2_0.bit4.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit4.obuf0  (.A(\storage_2_0.bit4.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_2_0.bit5.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit5.obuf0  (.A(\storage_2_0.bit5.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_2_0.bit6.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit6.obuf0  (.A(\storage_2_0.bit6.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_2_0.bit7.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit7.obuf0  (.A(\storage_2_0.bit7.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_2_0.cg  (.GATE(\storage_2_0.we0 ),
    .GCLK(\storage_2_0.gclock ),
    .CLK(\storage_2_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \storage_2_0.gcand  (.A(\storage_2_0.decoder0 ),
    .B(write_enable[0]),
    .X(\storage_2_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_2_0.select_inv_0  (.A(\storage_2_0.decoder0 ),
    .Y(\storage_2_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_2_0.clock_inv  (.A(clock),
    .Y(\storage_2_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \decoder2.and_layer0  (.A(\inv.bit [0]),
    .B(\decoder2.layer_in0 ),
    .X(\storage_2_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder2.and_layer1  (.A(bit[1]),
    .B(\inv.bit [2]),
    .X(\decoder2.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_3_0.bit0.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit0.obuf0  (.A(\storage_3_0.bit0.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_3_0.bit1.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit1.obuf0  (.A(\storage_3_0.bit1.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_3_0.bit2.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit2.obuf0  (.A(\storage_3_0.bit2.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_3_0.bit3.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit3.obuf0  (.A(\storage_3_0.bit3.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_3_0.bit4.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit4.obuf0  (.A(\storage_3_0.bit4.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_3_0.bit5.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit5.obuf0  (.A(\storage_3_0.bit5.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_3_0.bit6.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit6.obuf0  (.A(\storage_3_0.bit6.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_3_0.bit7.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit7.obuf0  (.A(\storage_3_0.bit7.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_3_0.cg  (.GATE(\storage_3_0.we0 ),
    .GCLK(\storage_3_0.gclock ),
    .CLK(\storage_3_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \storage_3_0.gcand  (.A(\storage_3_0.decoder0 ),
    .B(write_enable[0]),
    .X(\storage_3_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_3_0.select_inv_0  (.A(\storage_3_0.decoder0 ),
    .Y(\storage_3_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_3_0.clock_inv  (.A(clock),
    .Y(\storage_3_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \decoder3.and_layer0  (.A(\inv.bit [0]),
    .B(\decoder3.layer_in0 ),
    .X(\storage_3_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder3.and_layer1  (.A(bit[1]),
    .B(bit[2]),
    .X(\decoder3.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_4_0.bit0.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit0.obuf0  (.A(\storage_4_0.bit0.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_4_0.bit1.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit1.obuf0  (.A(\storage_4_0.bit1.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_4_0.bit2.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit2.obuf0  (.A(\storage_4_0.bit2.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_4_0.bit3.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit3.obuf0  (.A(\storage_4_0.bit3.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_4_0.bit4.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit4.obuf0  (.A(\storage_4_0.bit4.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_4_0.bit5.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit5.obuf0  (.A(\storage_4_0.bit5.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_4_0.bit6.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit6.obuf0  (.A(\storage_4_0.bit6.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_4_0.bit7.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit7.obuf0  (.A(\storage_4_0.bit7.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_4_0.cg  (.GATE(\storage_4_0.we0 ),
    .GCLK(\storage_4_0.gclock ),
    .CLK(\storage_4_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \storage_4_0.gcand  (.A(\storage_4_0.decoder0 ),
    .B(write_enable[0]),
    .X(\storage_4_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_4_0.select_inv_0  (.A(\storage_4_0.decoder0 ),
    .Y(\storage_4_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_4_0.clock_inv  (.A(clock),
    .Y(\storage_4_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \decoder4.and_layer0  (.A(bit[0]),
    .B(\decoder4.layer_in0 ),
    .X(\storage_4_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder4.and_layer1  (.A(\inv.bit [1]),
    .B(\inv.bit [2]),
    .X(\decoder4.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_5_0.bit0.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit0.obuf0  (.A(\storage_5_0.bit0.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_5_0.bit1.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit1.obuf0  (.A(\storage_5_0.bit1.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_5_0.bit2.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit2.obuf0  (.A(\storage_5_0.bit2.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_5_0.bit3.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit3.obuf0  (.A(\storage_5_0.bit3.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_5_0.bit4.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit4.obuf0  (.A(\storage_5_0.bit4.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_5_0.bit5.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit5.obuf0  (.A(\storage_5_0.bit5.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_5_0.bit6.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit6.obuf0  (.A(\storage_5_0.bit6.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_5_0.bit7.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit7.obuf0  (.A(\storage_5_0.bit7.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_5_0.cg  (.GATE(\storage_5_0.we0 ),
    .GCLK(\storage_5_0.gclock ),
    .CLK(\storage_5_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \storage_5_0.gcand  (.A(\storage_5_0.decoder0 ),
    .B(write_enable[0]),
    .X(\storage_5_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_5_0.select_inv_0  (.A(\storage_5_0.decoder0 ),
    .Y(\storage_5_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_5_0.clock_inv  (.A(clock),
    .Y(\storage_5_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \decoder5.and_layer0  (.A(bit[0]),
    .B(\decoder5.layer_in0 ),
    .X(\storage_5_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder5.and_layer1  (.A(\inv.bit [1]),
    .B(bit[2]),
    .X(\decoder5.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_6_0.bit0.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit0.obuf0  (.A(\storage_6_0.bit0.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_6_0.bit1.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit1.obuf0  (.A(\storage_6_0.bit1.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_6_0.bit2.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit2.obuf0  (.A(\storage_6_0.bit2.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_6_0.bit3.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit3.obuf0  (.A(\storage_6_0.bit3.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_6_0.bit4.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit4.obuf0  (.A(\storage_6_0.bit4.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_6_0.bit5.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit5.obuf0  (.A(\storage_6_0.bit5.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_6_0.bit6.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit6.obuf0  (.A(\storage_6_0.bit6.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_6_0.bit7.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit7.obuf0  (.A(\storage_6_0.bit7.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_6_0.cg  (.GATE(\storage_6_0.we0 ),
    .GCLK(\storage_6_0.gclock ),
    .CLK(\storage_6_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \storage_6_0.gcand  (.A(\storage_6_0.decoder0 ),
    .B(write_enable[0]),
    .X(\storage_6_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_6_0.select_inv_0  (.A(\storage_6_0.decoder0 ),
    .Y(\storage_6_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_6_0.clock_inv  (.A(clock),
    .Y(\storage_6_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \decoder6.and_layer0  (.A(bit[0]),
    .B(\decoder6.layer_in0 ),
    .X(\storage_6_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder6.and_layer1  (.A(bit[1]),
    .B(\inv.bit [2]),
    .X(\decoder6.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_7_0.bit0.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit0.obuf0  (.A(\storage_7_0.bit0.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_7_0.bit1.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit1.obuf0  (.A(\storage_7_0.bit1.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_7_0.bit2.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit2.obuf0  (.A(\storage_7_0.bit2.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_7_0.bit3.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit3.obuf0  (.A(\storage_7_0.bit3.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_7_0.bit4.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit4.obuf0  (.A(\storage_7_0.bit4.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_7_0.bit5.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit5.obuf0  (.A(\storage_7_0.bit5.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_7_0.bit6.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit6.obuf0  (.A(\storage_7_0.bit6.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_7_0.bit7.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit7.obuf0  (.A(\storage_7_0.bit7.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_7_0.cg  (.GATE(\storage_7_0.we0 ),
    .GCLK(\storage_7_0.gclock ),
    .CLK(\storage_7_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \storage_7_0.gcand  (.A(\storage_7_0.decoder0 ),
    .B(write_enable[0]),
    .X(\storage_7_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_7_0.select_inv_0  (.A(\storage_7_0.decoder0 ),
    .Y(\storage_7_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_7_0.clock_inv  (.A(clock),
    .Y(\storage_7_0.clock_b ));
 sky130_fd_sc_hd__and2_0 \decoder7.and_layer0  (.A(bit[0]),
    .B(\decoder7.layer_in0 ),
    .X(\storage_7_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder7.and_layer1  (.A(bit[1]),
    .B(bit[2]),
    .X(\decoder7.layer_in0 ));
 sky130_fd_sc_hd__buf_1 \buffer.in[0]  (.A(D[0]),
    .X(\D_nets[0].net ));
 sky130_fd_sc_hd__buf_1 \buffer.in[1]  (.A(D[1]),
    .X(\D_nets[1].net ));
 sky130_fd_sc_hd__buf_1 \buffer.in[2]  (.A(D[2]),
    .X(\D_nets[2].net ));
 sky130_fd_sc_hd__buf_1 \buffer.in[3]  (.A(D[3]),
    .X(\D_nets[3].net ));
 sky130_fd_sc_hd__buf_1 \buffer.in[4]  (.A(D[4]),
    .X(\D_nets[4].net ));
 sky130_fd_sc_hd__buf_1 \buffer.in[5]  (.A(D[5]),
    .X(\D_nets[5].net ));
 sky130_fd_sc_hd__buf_1 \buffer.in[6]  (.A(D[6]),
    .X(\D_nets[6].net ));
 sky130_fd_sc_hd__buf_1 \buffer.in[7]  (.A(D[7]),
    .X(\D_nets[7].net ));
 sky130_fd_sc_hd__clkinv_1 \decoder.inv_0  (.A(bit[0]),
    .Y(\inv.bit [0]));
 sky130_fd_sc_hd__clkinv_1 \decoder.inv_1  (.A(bit[1]),
    .Y(\inv.bit [1]));
 sky130_fd_sc_hd__clkinv_1 \decoder.inv_2  (.A(bit[2]),
    .Y(\inv.bit [2]));
endmodule
