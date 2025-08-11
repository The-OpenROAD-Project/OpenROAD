module RAM8x8 (clk,
    D,
    Q0,
    Q1,
    addr,
    we);
 input clk;
 input [7:0] D;
 output [7:0] Q0;
 output [7:0] Q1;
 input [2:0] addr;
 input [0:0] we;

 wire \D_nets[0].net ;
 wire \D_nets[1].net ;
 wire \D_nets[2].net ;
 wire \D_nets[3].net ;
 wire \D_nets[4].net ;
 wire \D_nets[5].net ;
 wire \D_nets[6].net ;
 wire \D_nets[7].net ;
 wire \storage_0_0.decoder0 ;
 wire \storage_0_0.decoder1 ;
 wire \storage_0_0.select0_b ;
 wire \storage_0_0.select1_b ;
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
 wire \decoder_0_0.layer_in0 ;
 wire \decoder_0_0.layer_in1 ;
 wire \storage_1_0.decoder0 ;
 wire \storage_1_0.decoder1 ;
 wire \storage_1_0.select0_b ;
 wire \storage_1_0.select1_b ;
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
 wire \decoder_1_0.layer_in0 ;
 wire \decoder_1_0.layer_in1 ;
 wire \storage_2_0.decoder0 ;
 wire \storage_2_0.decoder1 ;
 wire \storage_2_0.select0_b ;
 wire \storage_2_0.select1_b ;
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
 wire \decoder_2_0.layer_in0 ;
 wire \decoder_2_0.layer_in1 ;
 wire \storage_3_0.decoder0 ;
 wire \storage_3_0.decoder1 ;
 wire \storage_3_0.select0_b ;
 wire \storage_3_0.select1_b ;
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
 wire \decoder_3_0.layer_in0 ;
 wire \decoder_3_0.layer_in1 ;
 wire \storage_4_0.decoder0 ;
 wire \storage_4_0.decoder1 ;
 wire \storage_4_0.select0_b ;
 wire \storage_4_0.select1_b ;
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
 wire \decoder_4_0.layer_in0 ;
 wire \decoder_4_0.layer_in1 ;
 wire \storage_5_0.decoder0 ;
 wire \storage_5_0.decoder1 ;
 wire \storage_5_0.select0_b ;
 wire \storage_5_0.select1_b ;
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
 wire \decoder_5_0.layer_in0 ;
 wire \decoder_5_0.layer_in1 ;
 wire \storage_6_0.decoder0 ;
 wire \storage_6_0.decoder1 ;
 wire \storage_6_0.select0_b ;
 wire \storage_6_0.select1_b ;
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
 wire \decoder_6_0.layer_in0 ;
 wire \decoder_6_0.layer_in1 ;
 wire \storage_7_0.decoder0 ;
 wire \storage_7_0.decoder1 ;
 wire \storage_7_0.select0_b ;
 wire \storage_7_0.select1_b ;
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
 wire \decoder_7_0.layer_in0 ;
 wire \decoder_7_0.layer_in1 ;
 wire [2:0] \inv.addr ;

 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_0_0.bit0.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit0.obuf0  (.A(\storage_0_0.bit0.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q0[0]));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit0.obuf1  (.A(\storage_0_0.bit0.storage ),
    .TE_B(\storage_0_0.select1_b ),
    .Z(Q1[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_0_0.bit1.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit1.obuf0  (.A(\storage_0_0.bit1.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q0[1]));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit1.obuf1  (.A(\storage_0_0.bit1.storage ),
    .TE_B(\storage_0_0.select1_b ),
    .Z(Q1[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_0_0.bit2.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit2.obuf0  (.A(\storage_0_0.bit2.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q0[2]));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit2.obuf1  (.A(\storage_0_0.bit2.storage ),
    .TE_B(\storage_0_0.select1_b ),
    .Z(Q1[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_0_0.bit3.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit3.obuf0  (.A(\storage_0_0.bit3.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q0[3]));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit3.obuf1  (.A(\storage_0_0.bit3.storage ),
    .TE_B(\storage_0_0.select1_b ),
    .Z(Q1[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_0_0.bit4.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit4.obuf0  (.A(\storage_0_0.bit4.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q0[4]));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit4.obuf1  (.A(\storage_0_0.bit4.storage ),
    .TE_B(\storage_0_0.select1_b ),
    .Z(Q1[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_0_0.bit5.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit5.obuf0  (.A(\storage_0_0.bit5.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q0[5]));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit5.obuf1  (.A(\storage_0_0.bit5.storage ),
    .TE_B(\storage_0_0.select1_b ),
    .Z(Q1[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_0_0.bit6.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit6.obuf0  (.A(\storage_0_0.bit6.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q0[6]));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit6.obuf1  (.A(\storage_0_0.bit6.storage ),
    .TE_B(\storage_0_0.select1_b ),
    .Z(Q1[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_0_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_0_0.bit7.storage ),
    .GATE(\storage_0_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit7.obuf0  (.A(\storage_0_0.bit7.storage ),
    .TE_B(\storage_0_0.select0_b ),
    .Z(Q0[7]));
 sky130_fd_sc_hd__ebufn_1 \storage_0_0.bit7.obuf1  (.A(\storage_0_0.bit7.storage ),
    .TE_B(\storage_0_0.select1_b ),
    .Z(Q1[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_0_0.cg  (.GATE(\storage_0_0.we0 ),
    .GCLK(\storage_0_0.gclock ),
    .CLK(clk));
 sky130_fd_sc_hd__and2_0 \storage_0_0.gcand  (.A(\storage_0_0.decoder0 ),
    .B(we[0]),
    .X(\storage_0_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_0_0.select_inv_0  (.A(\storage_0_0.decoder0 ),
    .Y(\storage_0_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_0_0.select_inv_1  (.A(\storage_0_0.decoder1 ),
    .Y(\storage_0_0.select1_b ));
 sky130_fd_sc_hd__and2_0 \decoder_0_0.and_layer0  (.A(\inv.addr [0]),
    .B(\decoder_0_0.layer_in0 ),
    .X(\storage_0_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder_0_0.and_layer1  (.A(\inv.addr [1]),
    .B(\inv.addr [2]),
    .X(\decoder_0_0.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_1_0.bit0.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit0.obuf0  (.A(\storage_1_0.bit0.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q0[0]));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit0.obuf1  (.A(\storage_1_0.bit0.storage ),
    .TE_B(\storage_1_0.select1_b ),
    .Z(Q1[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_1_0.bit1.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit1.obuf0  (.A(\storage_1_0.bit1.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q0[1]));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit1.obuf1  (.A(\storage_1_0.bit1.storage ),
    .TE_B(\storage_1_0.select1_b ),
    .Z(Q1[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_1_0.bit2.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit2.obuf0  (.A(\storage_1_0.bit2.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q0[2]));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit2.obuf1  (.A(\storage_1_0.bit2.storage ),
    .TE_B(\storage_1_0.select1_b ),
    .Z(Q1[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_1_0.bit3.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit3.obuf0  (.A(\storage_1_0.bit3.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q0[3]));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit3.obuf1  (.A(\storage_1_0.bit3.storage ),
    .TE_B(\storage_1_0.select1_b ),
    .Z(Q1[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_1_0.bit4.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit4.obuf0  (.A(\storage_1_0.bit4.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q0[4]));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit4.obuf1  (.A(\storage_1_0.bit4.storage ),
    .TE_B(\storage_1_0.select1_b ),
    .Z(Q1[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_1_0.bit5.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit5.obuf0  (.A(\storage_1_0.bit5.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q0[5]));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit5.obuf1  (.A(\storage_1_0.bit5.storage ),
    .TE_B(\storage_1_0.select1_b ),
    .Z(Q1[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_1_0.bit6.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit6.obuf0  (.A(\storage_1_0.bit6.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q0[6]));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit6.obuf1  (.A(\storage_1_0.bit6.storage ),
    .TE_B(\storage_1_0.select1_b ),
    .Z(Q1[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_1_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_1_0.bit7.storage ),
    .GATE(\storage_1_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit7.obuf0  (.A(\storage_1_0.bit7.storage ),
    .TE_B(\storage_1_0.select0_b ),
    .Z(Q0[7]));
 sky130_fd_sc_hd__ebufn_1 \storage_1_0.bit7.obuf1  (.A(\storage_1_0.bit7.storage ),
    .TE_B(\storage_1_0.select1_b ),
    .Z(Q1[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_1_0.cg  (.GATE(\storage_1_0.we0 ),
    .GCLK(\storage_1_0.gclock ),
    .CLK(clk));
 sky130_fd_sc_hd__and2_0 \storage_1_0.gcand  (.A(\storage_1_0.decoder0 ),
    .B(we[0]),
    .X(\storage_1_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_1_0.select_inv_0  (.A(\storage_1_0.decoder0 ),
    .Y(\storage_1_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_1_0.select_inv_1  (.A(\storage_1_0.decoder1 ),
    .Y(\storage_1_0.select1_b ));
 sky130_fd_sc_hd__and2_0 \decoder_1_0.and_layer0  (.A(addr[0]),
    .B(\decoder_1_0.layer_in0 ),
    .X(\storage_1_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder_1_0.and_layer1  (.A(\inv.addr [1]),
    .B(\inv.addr [2]),
    .X(\decoder_1_0.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_2_0.bit0.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit0.obuf0  (.A(\storage_2_0.bit0.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q0[0]));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit0.obuf1  (.A(\storage_2_0.bit0.storage ),
    .TE_B(\storage_2_0.select1_b ),
    .Z(Q1[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_2_0.bit1.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit1.obuf0  (.A(\storage_2_0.bit1.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q0[1]));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit1.obuf1  (.A(\storage_2_0.bit1.storage ),
    .TE_B(\storage_2_0.select1_b ),
    .Z(Q1[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_2_0.bit2.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit2.obuf0  (.A(\storage_2_0.bit2.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q0[2]));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit2.obuf1  (.A(\storage_2_0.bit2.storage ),
    .TE_B(\storage_2_0.select1_b ),
    .Z(Q1[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_2_0.bit3.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit3.obuf0  (.A(\storage_2_0.bit3.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q0[3]));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit3.obuf1  (.A(\storage_2_0.bit3.storage ),
    .TE_B(\storage_2_0.select1_b ),
    .Z(Q1[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_2_0.bit4.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit4.obuf0  (.A(\storage_2_0.bit4.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q0[4]));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit4.obuf1  (.A(\storage_2_0.bit4.storage ),
    .TE_B(\storage_2_0.select1_b ),
    .Z(Q1[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_2_0.bit5.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit5.obuf0  (.A(\storage_2_0.bit5.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q0[5]));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit5.obuf1  (.A(\storage_2_0.bit5.storage ),
    .TE_B(\storage_2_0.select1_b ),
    .Z(Q1[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_2_0.bit6.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit6.obuf0  (.A(\storage_2_0.bit6.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q0[6]));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit6.obuf1  (.A(\storage_2_0.bit6.storage ),
    .TE_B(\storage_2_0.select1_b ),
    .Z(Q1[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_2_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_2_0.bit7.storage ),
    .GATE(\storage_2_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit7.obuf0  (.A(\storage_2_0.bit7.storage ),
    .TE_B(\storage_2_0.select0_b ),
    .Z(Q0[7]));
 sky130_fd_sc_hd__ebufn_1 \storage_2_0.bit7.obuf1  (.A(\storage_2_0.bit7.storage ),
    .TE_B(\storage_2_0.select1_b ),
    .Z(Q1[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_2_0.cg  (.GATE(\storage_2_0.we0 ),
    .GCLK(\storage_2_0.gclock ),
    .CLK(clk));
 sky130_fd_sc_hd__and2_0 \storage_2_0.gcand  (.A(\storage_2_0.decoder0 ),
    .B(we[0]),
    .X(\storage_2_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_2_0.select_inv_0  (.A(\storage_2_0.decoder0 ),
    .Y(\storage_2_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_2_0.select_inv_1  (.A(\storage_2_0.decoder1 ),
    .Y(\storage_2_0.select1_b ));
 sky130_fd_sc_hd__and2_0 \decoder_2_0.and_layer0  (.A(\inv.addr [0]),
    .B(\decoder_2_0.layer_in0 ),
    .X(\storage_2_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder_2_0.and_layer1  (.A(addr[1]),
    .B(\inv.addr [2]),
    .X(\decoder_2_0.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_3_0.bit0.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit0.obuf0  (.A(\storage_3_0.bit0.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q0[0]));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit0.obuf1  (.A(\storage_3_0.bit0.storage ),
    .TE_B(\storage_3_0.select1_b ),
    .Z(Q1[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_3_0.bit1.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit1.obuf0  (.A(\storage_3_0.bit1.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q0[1]));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit1.obuf1  (.A(\storage_3_0.bit1.storage ),
    .TE_B(\storage_3_0.select1_b ),
    .Z(Q1[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_3_0.bit2.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit2.obuf0  (.A(\storage_3_0.bit2.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q0[2]));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit2.obuf1  (.A(\storage_3_0.bit2.storage ),
    .TE_B(\storage_3_0.select1_b ),
    .Z(Q1[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_3_0.bit3.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit3.obuf0  (.A(\storage_3_0.bit3.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q0[3]));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit3.obuf1  (.A(\storage_3_0.bit3.storage ),
    .TE_B(\storage_3_0.select1_b ),
    .Z(Q1[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_3_0.bit4.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit4.obuf0  (.A(\storage_3_0.bit4.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q0[4]));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit4.obuf1  (.A(\storage_3_0.bit4.storage ),
    .TE_B(\storage_3_0.select1_b ),
    .Z(Q1[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_3_0.bit5.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit5.obuf0  (.A(\storage_3_0.bit5.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q0[5]));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit5.obuf1  (.A(\storage_3_0.bit5.storage ),
    .TE_B(\storage_3_0.select1_b ),
    .Z(Q1[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_3_0.bit6.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit6.obuf0  (.A(\storage_3_0.bit6.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q0[6]));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit6.obuf1  (.A(\storage_3_0.bit6.storage ),
    .TE_B(\storage_3_0.select1_b ),
    .Z(Q1[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_3_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_3_0.bit7.storage ),
    .GATE(\storage_3_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit7.obuf0  (.A(\storage_3_0.bit7.storage ),
    .TE_B(\storage_3_0.select0_b ),
    .Z(Q0[7]));
 sky130_fd_sc_hd__ebufn_1 \storage_3_0.bit7.obuf1  (.A(\storage_3_0.bit7.storage ),
    .TE_B(\storage_3_0.select1_b ),
    .Z(Q1[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_3_0.cg  (.GATE(\storage_3_0.we0 ),
    .GCLK(\storage_3_0.gclock ),
    .CLK(clk));
 sky130_fd_sc_hd__and2_0 \storage_3_0.gcand  (.A(\storage_3_0.decoder0 ),
    .B(we[0]),
    .X(\storage_3_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_3_0.select_inv_0  (.A(\storage_3_0.decoder0 ),
    .Y(\storage_3_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_3_0.select_inv_1  (.A(\storage_3_0.decoder1 ),
    .Y(\storage_3_0.select1_b ));
 sky130_fd_sc_hd__and2_0 \decoder_3_0.and_layer0  (.A(addr[0]),
    .B(\decoder_3_0.layer_in0 ),
    .X(\storage_3_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder_3_0.and_layer1  (.A(addr[1]),
    .B(\inv.addr [2]),
    .X(\decoder_3_0.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_4_0.bit0.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit0.obuf0  (.A(\storage_4_0.bit0.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q0[0]));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit0.obuf1  (.A(\storage_4_0.bit0.storage ),
    .TE_B(\storage_4_0.select1_b ),
    .Z(Q1[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_4_0.bit1.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit1.obuf0  (.A(\storage_4_0.bit1.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q0[1]));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit1.obuf1  (.A(\storage_4_0.bit1.storage ),
    .TE_B(\storage_4_0.select1_b ),
    .Z(Q1[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_4_0.bit2.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit2.obuf0  (.A(\storage_4_0.bit2.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q0[2]));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit2.obuf1  (.A(\storage_4_0.bit2.storage ),
    .TE_B(\storage_4_0.select1_b ),
    .Z(Q1[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_4_0.bit3.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit3.obuf0  (.A(\storage_4_0.bit3.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q0[3]));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit3.obuf1  (.A(\storage_4_0.bit3.storage ),
    .TE_B(\storage_4_0.select1_b ),
    .Z(Q1[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_4_0.bit4.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit4.obuf0  (.A(\storage_4_0.bit4.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q0[4]));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit4.obuf1  (.A(\storage_4_0.bit4.storage ),
    .TE_B(\storage_4_0.select1_b ),
    .Z(Q1[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_4_0.bit5.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit5.obuf0  (.A(\storage_4_0.bit5.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q0[5]));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit5.obuf1  (.A(\storage_4_0.bit5.storage ),
    .TE_B(\storage_4_0.select1_b ),
    .Z(Q1[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_4_0.bit6.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit6.obuf0  (.A(\storage_4_0.bit6.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q0[6]));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit6.obuf1  (.A(\storage_4_0.bit6.storage ),
    .TE_B(\storage_4_0.select1_b ),
    .Z(Q1[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_4_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_4_0.bit7.storage ),
    .GATE(\storage_4_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit7.obuf0  (.A(\storage_4_0.bit7.storage ),
    .TE_B(\storage_4_0.select0_b ),
    .Z(Q0[7]));
 sky130_fd_sc_hd__ebufn_1 \storage_4_0.bit7.obuf1  (.A(\storage_4_0.bit7.storage ),
    .TE_B(\storage_4_0.select1_b ),
    .Z(Q1[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_4_0.cg  (.GATE(\storage_4_0.we0 ),
    .GCLK(\storage_4_0.gclock ),
    .CLK(clk));
 sky130_fd_sc_hd__and2_0 \storage_4_0.gcand  (.A(\storage_4_0.decoder0 ),
    .B(we[0]),
    .X(\storage_4_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_4_0.select_inv_0  (.A(\storage_4_0.decoder0 ),
    .Y(\storage_4_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_4_0.select_inv_1  (.A(\storage_4_0.decoder1 ),
    .Y(\storage_4_0.select1_b ));
 sky130_fd_sc_hd__and2_0 \decoder_4_0.and_layer0  (.A(\inv.addr [0]),
    .B(\decoder_4_0.layer_in0 ),
    .X(\storage_4_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder_4_0.and_layer1  (.A(\inv.addr [1]),
    .B(addr[2]),
    .X(\decoder_4_0.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_5_0.bit0.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit0.obuf0  (.A(\storage_5_0.bit0.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q0[0]));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit0.obuf1  (.A(\storage_5_0.bit0.storage ),
    .TE_B(\storage_5_0.select1_b ),
    .Z(Q1[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_5_0.bit1.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit1.obuf0  (.A(\storage_5_0.bit1.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q0[1]));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit1.obuf1  (.A(\storage_5_0.bit1.storage ),
    .TE_B(\storage_5_0.select1_b ),
    .Z(Q1[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_5_0.bit2.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit2.obuf0  (.A(\storage_5_0.bit2.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q0[2]));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit2.obuf1  (.A(\storage_5_0.bit2.storage ),
    .TE_B(\storage_5_0.select1_b ),
    .Z(Q1[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_5_0.bit3.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit3.obuf0  (.A(\storage_5_0.bit3.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q0[3]));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit3.obuf1  (.A(\storage_5_0.bit3.storage ),
    .TE_B(\storage_5_0.select1_b ),
    .Z(Q1[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_5_0.bit4.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit4.obuf0  (.A(\storage_5_0.bit4.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q0[4]));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit4.obuf1  (.A(\storage_5_0.bit4.storage ),
    .TE_B(\storage_5_0.select1_b ),
    .Z(Q1[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_5_0.bit5.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit5.obuf0  (.A(\storage_5_0.bit5.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q0[5]));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit5.obuf1  (.A(\storage_5_0.bit5.storage ),
    .TE_B(\storage_5_0.select1_b ),
    .Z(Q1[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_5_0.bit6.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit6.obuf0  (.A(\storage_5_0.bit6.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q0[6]));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit6.obuf1  (.A(\storage_5_0.bit6.storage ),
    .TE_B(\storage_5_0.select1_b ),
    .Z(Q1[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_5_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_5_0.bit7.storage ),
    .GATE(\storage_5_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit7.obuf0  (.A(\storage_5_0.bit7.storage ),
    .TE_B(\storage_5_0.select0_b ),
    .Z(Q0[7]));
 sky130_fd_sc_hd__ebufn_1 \storage_5_0.bit7.obuf1  (.A(\storage_5_0.bit7.storage ),
    .TE_B(\storage_5_0.select1_b ),
    .Z(Q1[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_5_0.cg  (.GATE(\storage_5_0.we0 ),
    .GCLK(\storage_5_0.gclock ),
    .CLK(clk));
 sky130_fd_sc_hd__and2_0 \storage_5_0.gcand  (.A(\storage_5_0.decoder0 ),
    .B(we[0]),
    .X(\storage_5_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_5_0.select_inv_0  (.A(\storage_5_0.decoder0 ),
    .Y(\storage_5_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_5_0.select_inv_1  (.A(\storage_5_0.decoder1 ),
    .Y(\storage_5_0.select1_b ));
 sky130_fd_sc_hd__and2_0 \decoder_5_0.and_layer0  (.A(addr[0]),
    .B(\decoder_5_0.layer_in0 ),
    .X(\storage_5_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder_5_0.and_layer1  (.A(\inv.addr [1]),
    .B(addr[2]),
    .X(\decoder_5_0.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_6_0.bit0.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit0.obuf0  (.A(\storage_6_0.bit0.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q0[0]));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit0.obuf1  (.A(\storage_6_0.bit0.storage ),
    .TE_B(\storage_6_0.select1_b ),
    .Z(Q1[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_6_0.bit1.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit1.obuf0  (.A(\storage_6_0.bit1.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q0[1]));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit1.obuf1  (.A(\storage_6_0.bit1.storage ),
    .TE_B(\storage_6_0.select1_b ),
    .Z(Q1[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_6_0.bit2.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit2.obuf0  (.A(\storage_6_0.bit2.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q0[2]));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit2.obuf1  (.A(\storage_6_0.bit2.storage ),
    .TE_B(\storage_6_0.select1_b ),
    .Z(Q1[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_6_0.bit3.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit3.obuf0  (.A(\storage_6_0.bit3.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q0[3]));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit3.obuf1  (.A(\storage_6_0.bit3.storage ),
    .TE_B(\storage_6_0.select1_b ),
    .Z(Q1[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_6_0.bit4.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit4.obuf0  (.A(\storage_6_0.bit4.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q0[4]));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit4.obuf1  (.A(\storage_6_0.bit4.storage ),
    .TE_B(\storage_6_0.select1_b ),
    .Z(Q1[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_6_0.bit5.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit5.obuf0  (.A(\storage_6_0.bit5.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q0[5]));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit5.obuf1  (.A(\storage_6_0.bit5.storage ),
    .TE_B(\storage_6_0.select1_b ),
    .Z(Q1[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_6_0.bit6.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit6.obuf0  (.A(\storage_6_0.bit6.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q0[6]));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit6.obuf1  (.A(\storage_6_0.bit6.storage ),
    .TE_B(\storage_6_0.select1_b ),
    .Z(Q1[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_6_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_6_0.bit7.storage ),
    .GATE(\storage_6_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit7.obuf0  (.A(\storage_6_0.bit7.storage ),
    .TE_B(\storage_6_0.select0_b ),
    .Z(Q0[7]));
 sky130_fd_sc_hd__ebufn_1 \storage_6_0.bit7.obuf1  (.A(\storage_6_0.bit7.storage ),
    .TE_B(\storage_6_0.select1_b ),
    .Z(Q1[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_6_0.cg  (.GATE(\storage_6_0.we0 ),
    .GCLK(\storage_6_0.gclock ),
    .CLK(clk));
 sky130_fd_sc_hd__and2_0 \storage_6_0.gcand  (.A(\storage_6_0.decoder0 ),
    .B(we[0]),
    .X(\storage_6_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_6_0.select_inv_0  (.A(\storage_6_0.decoder0 ),
    .Y(\storage_6_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_6_0.select_inv_1  (.A(\storage_6_0.decoder1 ),
    .Y(\storage_6_0.select1_b ));
 sky130_fd_sc_hd__and2_0 \decoder_6_0.and_layer0  (.A(\inv.addr [0]),
    .B(\decoder_6_0.layer_in0 ),
    .X(\storage_6_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder_6_0.and_layer1  (.A(addr[1]),
    .B(addr[2]),
    .X(\decoder_6_0.layer_in0 ));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit0.bit  (.D(\D_nets[0].net ),
    .Q(\storage_7_0.bit0.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit0.obuf0  (.A(\storage_7_0.bit0.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q0[0]));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit0.obuf1  (.A(\storage_7_0.bit0.storage ),
    .TE_B(\storage_7_0.select1_b ),
    .Z(Q1[0]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit1.bit  (.D(\D_nets[1].net ),
    .Q(\storage_7_0.bit1.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit1.obuf0  (.A(\storage_7_0.bit1.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q0[1]));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit1.obuf1  (.A(\storage_7_0.bit1.storage ),
    .TE_B(\storage_7_0.select1_b ),
    .Z(Q1[1]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit2.bit  (.D(\D_nets[2].net ),
    .Q(\storage_7_0.bit2.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit2.obuf0  (.A(\storage_7_0.bit2.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q0[2]));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit2.obuf1  (.A(\storage_7_0.bit2.storage ),
    .TE_B(\storage_7_0.select1_b ),
    .Z(Q1[2]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit3.bit  (.D(\D_nets[3].net ),
    .Q(\storage_7_0.bit3.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit3.obuf0  (.A(\storage_7_0.bit3.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q0[3]));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit3.obuf1  (.A(\storage_7_0.bit3.storage ),
    .TE_B(\storage_7_0.select1_b ),
    .Z(Q1[3]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit4.bit  (.D(\D_nets[4].net ),
    .Q(\storage_7_0.bit4.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit4.obuf0  (.A(\storage_7_0.bit4.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q0[4]));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit4.obuf1  (.A(\storage_7_0.bit4.storage ),
    .TE_B(\storage_7_0.select1_b ),
    .Z(Q1[4]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit5.bit  (.D(\D_nets[5].net ),
    .Q(\storage_7_0.bit5.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit5.obuf0  (.A(\storage_7_0.bit5.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q0[5]));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit5.obuf1  (.A(\storage_7_0.bit5.storage ),
    .TE_B(\storage_7_0.select1_b ),
    .Z(Q1[5]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit6.bit  (.D(\D_nets[6].net ),
    .Q(\storage_7_0.bit6.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit6.obuf0  (.A(\storage_7_0.bit6.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q0[6]));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit6.obuf1  (.A(\storage_7_0.bit6.storage ),
    .TE_B(\storage_7_0.select1_b ),
    .Z(Q1[6]));
 sky130_fd_sc_hd__dlxtp_1 \storage_7_0.bit7.bit  (.D(\D_nets[7].net ),
    .Q(\storage_7_0.bit7.storage ),
    .GATE(\storage_7_0.gclock ));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit7.obuf0  (.A(\storage_7_0.bit7.storage ),
    .TE_B(\storage_7_0.select0_b ),
    .Z(Q0[7]));
 sky130_fd_sc_hd__ebufn_1 \storage_7_0.bit7.obuf1  (.A(\storage_7_0.bit7.storage ),
    .TE_B(\storage_7_0.select1_b ),
    .Z(Q1[7]));
 sky130_fd_sc_hd__dlclkp_1 \storage_7_0.cg  (.GATE(\storage_7_0.we0 ),
    .GCLK(\storage_7_0.gclock ),
    .CLK(clk));
 sky130_fd_sc_hd__and2_0 \storage_7_0.gcand  (.A(\storage_7_0.decoder0 ),
    .B(we[0]),
    .X(\storage_7_0.we0 ));
 sky130_fd_sc_hd__clkinv_1 \storage_7_0.select_inv_0  (.A(\storage_7_0.decoder0 ),
    .Y(\storage_7_0.select0_b ));
 sky130_fd_sc_hd__clkinv_1 \storage_7_0.select_inv_1  (.A(\storage_7_0.decoder1 ),
    .Y(\storage_7_0.select1_b ));
 sky130_fd_sc_hd__and2_0 \decoder_7_0.and_layer0  (.A(addr[0]),
    .B(\decoder_7_0.layer_in0 ),
    .X(\storage_7_0.decoder0 ));
 sky130_fd_sc_hd__and2_0 \decoder_7_0.and_layer1  (.A(addr[1]),
    .B(addr[2]),
    .X(\decoder_7_0.layer_in0 ));
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
 sky130_fd_sc_hd__clkinv_1 \decoder.inv_2  (.A(addr[2]),
    .Y(\inv.addr [2]));
 sky130_fd_sc_hd__clkinv_1 \decoder.inv_1  (.A(addr[1]),
    .Y(\inv.addr [1]));
 sky130_fd_sc_hd__clkinv_1 \decoder.inv_0  (.A(addr[0]),
    .Y(\inv.addr [0]));
endmodule
