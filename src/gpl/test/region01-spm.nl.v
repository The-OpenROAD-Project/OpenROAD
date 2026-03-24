module spm (clk,
    rst,
    x,
    y,
    a);
 input clk;
 input rst;
 input x;
 output y;
 input [31:0] a;

 wire _000_;
 wire _001_;
 wire _002_;
 wire _003_;
 wire _004_;
 wire _005_;
 wire _006_;
 wire _007_;
 wire _008_;
 wire _009_;
 wire _010_;
 wire _011_;
 wire _012_;
 wire _013_;
 wire _014_;
 wire _015_;
 wire _016_;
 wire _017_;
 wire _018_;
 wire _019_;
 wire _020_;
 wire _021_;
 wire _022_;
 wire _023_;
 wire _024_;
 wire _025_;
 wire _026_;
 wire _027_;
 wire _028_;
 wire _029_;
 wire _030_;
 wire _031_;
 wire _032_;
 wire _033_;
 wire _034_;
 wire _035_;
 wire _036_;
 wire _037_;
 wire _038_;
 wire _039_;
 wire _040_;
 wire _041_;
 wire _042_;
 wire _043_;
 wire _044_;
 wire _045_;
 wire _046_;
 wire _047_;
 wire _048_;
 wire _049_;
 wire _050_;
 wire _051_;
 wire _052_;
 wire _053_;
 wire _054_;
 wire _055_;
 wire _056_;
 wire _057_;
 wire _058_;
 wire _059_;
 wire _060_;
 wire _061_;
 wire _062_;
 wire _063_;
 wire _064_;
 wire _065_;
 wire _066_;
 wire _067_;
 wire _068_;
 wire _069_;
 wire _070_;
 wire _071_;
 wire _072_;
 wire _073_;
 wire _074_;
 wire _075_;
 wire _076_;
 wire _077_;
 wire _078_;
 wire _079_;
 wire _080_;
 wire _081_;
 wire _082_;
 wire _083_;
 wire _084_;
 wire _085_;
 wire _086_;
 wire _087_;
 wire _088_;
 wire _089_;
 wire _090_;
 wire _091_;
 wire _092_;
 wire _093_;
 wire \dsa[0].lastCarry ;
 wire \dsa[0].lastCarry_next ;
 wire \dsa[0].y_out ;
 wire \dsa[0].y_out_next ;
 wire \dsa[10].lastCarry ;
 wire \dsa[10].lastCarry_next ;
 wire \dsa[10].y_in ;
 wire \dsa[10].y_out ;
 wire \dsa[10].y_out_next ;
 wire \dsa[11].lastCarry ;
 wire \dsa[11].lastCarry_next ;
 wire \dsa[11].y_out ;
 wire \dsa[11].y_out_next ;
 wire \dsa[12].lastCarry ;
 wire \dsa[12].lastCarry_next ;
 wire \dsa[12].y_out ;
 wire \dsa[12].y_out_next ;
 wire \dsa[13].lastCarry ;
 wire \dsa[13].lastCarry_next ;
 wire \dsa[13].y_out ;
 wire \dsa[13].y_out_next ;
 wire \dsa[14].lastCarry ;
 wire \dsa[14].lastCarry_next ;
 wire \dsa[14].y_out ;
 wire \dsa[14].y_out_next ;
 wire \dsa[15].lastCarry ;
 wire \dsa[15].lastCarry_next ;
 wire \dsa[15].y_out ;
 wire \dsa[15].y_out_next ;
 wire \dsa[16].lastCarry ;
 wire \dsa[16].lastCarry_next ;
 wire \dsa[16].y_out ;
 wire \dsa[16].y_out_next ;
 wire \dsa[17].lastCarry ;
 wire \dsa[17].lastCarry_next ;
 wire \dsa[17].y_out ;
 wire \dsa[17].y_out_next ;
 wire \dsa[18].lastCarry ;
 wire \dsa[18].lastCarry_next ;
 wire \dsa[18].y_out ;
 wire \dsa[18].y_out_next ;
 wire \dsa[19].lastCarry ;
 wire \dsa[19].lastCarry_next ;
 wire \dsa[19].y_out ;
 wire \dsa[19].y_out_next ;
 wire \dsa[1].lastCarry ;
 wire \dsa[1].lastCarry_next ;
 wire \dsa[1].y_out ;
 wire \dsa[1].y_out_next ;
 wire \dsa[20].lastCarry ;
 wire \dsa[20].lastCarry_next ;
 wire \dsa[20].y_out ;
 wire \dsa[20].y_out_next ;
 wire \dsa[21].lastCarry ;
 wire \dsa[21].lastCarry_next ;
 wire \dsa[21].y_out ;
 wire \dsa[21].y_out_next ;
 wire \dsa[22].lastCarry ;
 wire \dsa[22].lastCarry_next ;
 wire \dsa[22].y_out ;
 wire \dsa[22].y_out_next ;
 wire \dsa[23].lastCarry ;
 wire \dsa[23].lastCarry_next ;
 wire \dsa[23].y_out ;
 wire \dsa[23].y_out_next ;
 wire \dsa[24].lastCarry ;
 wire \dsa[24].lastCarry_next ;
 wire \dsa[24].y_out ;
 wire \dsa[24].y_out_next ;
 wire \dsa[25].lastCarry ;
 wire \dsa[25].lastCarry_next ;
 wire \dsa[25].y_out ;
 wire \dsa[25].y_out_next ;
 wire \dsa[26].lastCarry ;
 wire \dsa[26].lastCarry_next ;
 wire \dsa[26].y_out ;
 wire \dsa[26].y_out_next ;
 wire \dsa[27].lastCarry ;
 wire \dsa[27].lastCarry_next ;
 wire \dsa[27].y_out ;
 wire \dsa[27].y_out_next ;
 wire \dsa[28].lastCarry ;
 wire \dsa[28].lastCarry_next ;
 wire \dsa[28].y_out ;
 wire \dsa[28].y_out_next ;
 wire \dsa[29].lastCarry ;
 wire \dsa[29].lastCarry_next ;
 wire \dsa[29].y_out ;
 wire \dsa[29].y_out_next ;
 wire \dsa[2].lastCarry ;
 wire \dsa[2].lastCarry_next ;
 wire \dsa[2].y_out ;
 wire \dsa[2].y_out_next ;
 wire \dsa[30].lastCarry ;
 wire \dsa[30].lastCarry_next ;
 wire \dsa[30].y_out ;
 wire \dsa[30].y_out_next ;
 wire \dsa[31].lastCarry ;
 wire \dsa[31].lastCarry_next ;
 wire \dsa[31].y_out_next ;
 wire \dsa[3].lastCarry ;
 wire \dsa[3].lastCarry_next ;
 wire \dsa[3].y_out ;
 wire \dsa[3].y_out_next ;
 wire \dsa[4].lastCarry ;
 wire \dsa[4].lastCarry_next ;
 wire \dsa[4].y_out ;
 wire \dsa[4].y_out_next ;
 wire \dsa[5].lastCarry ;
 wire \dsa[5].lastCarry_next ;
 wire \dsa[5].y_out ;
 wire \dsa[5].y_out_next ;
 wire \dsa[6].lastCarry ;
 wire \dsa[6].lastCarry_next ;
 wire \dsa[6].y_out ;
 wire \dsa[6].y_out_next ;
 wire \dsa[7].lastCarry ;
 wire \dsa[7].lastCarry_next ;
 wire \dsa[7].y_out ;
 wire \dsa[7].y_out_next ;
 wire \dsa[8].lastCarry ;
 wire \dsa[8].lastCarry_next ;
 wire \dsa[8].y_out ;
 wire \dsa[8].y_out_next ;
 wire \dsa[9].lastCarry ;
 wire \dsa[9].lastCarry_next ;
 wire \dsa[9].y_out_next ;
 wire net1;
 wire net2;
 wire net3;
 wire net4;
 wire net5;
 wire net6;
 wire net7;
 wire net8;
 wire net9;
 wire net10;
 wire net11;
 wire net12;
 wire net13;
 wire net14;
 wire net15;
 wire net16;
 wire net17;
 wire net18;
 wire net19;
 wire net20;
 wire net21;
 wire net22;
 wire net23;
 wire net24;
 wire net25;
 wire net26;
 wire net27;
 wire net28;
 wire net29;
 wire net30;
 wire net31;
 wire net32;
 wire net33;
 wire net34;
 wire net35;
 wire net36;
 wire net37;
 wire net38;
 wire net39;
 wire net40;
 wire net41;
 wire net42;
 wire net43;
 wire net44;
 wire net45;
 wire net46;
 wire net47;
 wire net48;
 wire net49;
 wire net50;
 wire net51;
 wire net52;
 wire clknet_0_clk;
 wire clknet_3_0__leaf_clk;
 wire clknet_3_1__leaf_clk;
 wire clknet_3_2__leaf_clk;
 wire clknet_3_3__leaf_clk;
 wire clknet_3_4__leaf_clk;
 wire clknet_3_5__leaf_clk;
 wire clknet_3_6__leaf_clk;
 wire clknet_3_7__leaf_clk;
 wire net53;
 wire net54;
 wire net55;
 wire net56;
 wire net57;
 wire net58;
 wire net59;
 wire net60;
 wire net61;
 wire net62;
 wire net63;
 wire net64;
 wire net65;
 wire net66;
 wire net67;
 wire net68;
 wire net69;
 wire net70;
 wire net71;
 wire net72;
 wire net73;
 wire net74;
 wire net75;
 wire net76;
 wire net77;
 wire net78;
 wire net79;
 wire net80;
 wire net81;
 wire net82;
 wire net83;
 wire net84;

 sky130_fd_sc_hd__xnor2_1 _094_ (.A(net75),
    .B(_029_),
    .Y(\dsa[10].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _095_ (.A1(net40),
    .A2(net30),
    .B1(\dsa[24].lastCarry ),
    .X(_030_));
 sky130_fd_sc_hd__nand3_1 _096_ (.A(net40),
    .B(net30),
    .C(\dsa[24].lastCarry ),
    .Y(_031_));
 sky130_fd_sc_hd__nand2_1 _097_ (.A(_030_),
    .B(_031_),
    .Y(_032_));
 sky130_fd_sc_hd__xnor2_1 _098_ (.A(net74),
    .B(_032_),
    .Y(\dsa[24].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _099_ (.A1(net38),
    .A2(net16),
    .B1(\dsa[8].lastCarry ),
    .X(_033_));
 sky130_fd_sc_hd__nand3_1 _100_ (.A(net38),
    .B(net16),
    .C(\dsa[8].lastCarry ),
    .Y(_034_));
 sky130_fd_sc_hd__nand2_1 _101_ (.A(_033_),
    .B(_034_),
    .Y(_035_));
 sky130_fd_sc_hd__xnor2_1 _102_ (.A(net82),
    .B(_035_),
    .Y(\dsa[8].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _103_ (.A1(net40),
    .A2(net12),
    .B1(\dsa[30].lastCarry ),
    .X(_036_));
 sky130_fd_sc_hd__nand3_1 _104_ (.A(net40),
    .B(net12),
    .C(\dsa[30].lastCarry ),
    .Y(_037_));
 sky130_fd_sc_hd__nand2_1 _105_ (.A(_036_),
    .B(_037_),
    .Y(_038_));
 sky130_fd_sc_hd__xnor2_1 _106_ (.A(net54),
    .B(_038_),
    .Y(\dsa[30].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _107_ (.A1(net36),
    .A2(net17),
    .B1(\dsa[7].lastCarry ),
    .X(_039_));
 sky130_fd_sc_hd__nand3_1 _108_ (.A(net36),
    .B(net17),
    .C(\dsa[7].lastCarry ),
    .Y(_040_));
 sky130_fd_sc_hd__nand2_1 _109_ (.A(_039_),
    .B(_040_),
    .Y(_041_));
 sky130_fd_sc_hd__xnor2_1 _110_ (.A(net65),
    .B(_041_),
    .Y(\dsa[7].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _111_ (.A1(net41),
    .A2(net29),
    .B1(\dsa[25].lastCarry ),
    .X(_042_));
 sky130_fd_sc_hd__nand3_1 _112_ (.A(net41),
    .B(net29),
    .C(\dsa[25].lastCarry ),
    .Y(_043_));
 sky130_fd_sc_hd__nand2_1 _113_ (.A(_042_),
    .B(_043_),
    .Y(_044_));
 sky130_fd_sc_hd__xnor2_1 _114_ (.A(net71),
    .B(_044_),
    .Y(\dsa[25].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _115_ (.A1(net36),
    .A2(net18),
    .B1(\dsa[6].lastCarry ),
    .X(_045_));
 sky130_fd_sc_hd__nand3_1 _116_ (.A(net36),
    .B(net18),
    .C(\dsa[6].lastCarry ),
    .Y(_046_));
 sky130_fd_sc_hd__nand2_1 _117_ (.A(_045_),
    .B(_046_),
    .Y(_047_));
 sky130_fd_sc_hd__xnor2_1 _118_ (.A(net72),
    .B(_047_),
    .Y(\dsa[6].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _119_ (.A1(net40),
    .A2(net26),
    .B1(\dsa[28].lastCarry ),
    .X(_048_));
 sky130_fd_sc_hd__nand3_1 _120_ (.A(net40),
    .B(net26),
    .C(\dsa[28].lastCarry ),
    .Y(_049_));
 sky130_fd_sc_hd__nand2_1 _121_ (.A(_048_),
    .B(_049_),
    .Y(_050_));
 sky130_fd_sc_hd__xnor2_1 _122_ (.A(net56),
    .B(_050_),
    .Y(\dsa[28].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _123_ (.A1(net37),
    .A2(net19),
    .B1(\dsa[5].lastCarry ),
    .X(_051_));
 sky130_fd_sc_hd__nand3_1 _124_ (.A(net37),
    .B(net19),
    .C(\dsa[5].lastCarry ),
    .Y(_052_));
 sky130_fd_sc_hd__nand2_1 _125_ (.A(_051_),
    .B(_052_),
    .Y(_053_));
 sky130_fd_sc_hd__xnor2_1 _126_ (.A(net79),
    .B(_053_),
    .Y(\dsa[5].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _127_ (.A1(net41),
    .A2(net28),
    .B1(\dsa[26].lastCarry ),
    .X(_054_));
 sky130_fd_sc_hd__nand3_1 _128_ (.A(net41),
    .B(net28),
    .C(\dsa[26].lastCarry ),
    .Y(_055_));
 sky130_fd_sc_hd__nand2_1 _129_ (.A(_054_),
    .B(_055_),
    .Y(_056_));
 sky130_fd_sc_hd__xnor2_1 _130_ (.A(net59),
    .B(_056_),
    .Y(\dsa[26].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _131_ (.A1(net37),
    .A2(net20),
    .B1(\dsa[4].lastCarry ),
    .X(_057_));
 sky130_fd_sc_hd__nand3_1 _132_ (.A(net37),
    .B(net20),
    .C(\dsa[4].lastCarry ),
    .Y(_058_));
 sky130_fd_sc_hd__nand2_1 _133_ (.A(_057_),
    .B(_058_),
    .Y(_059_));
 sky130_fd_sc_hd__xnor2_1 _134_ (.A(net61),
    .B(_059_),
    .Y(\dsa[4].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _135_ (.A1(net36),
    .A2(net21),
    .B1(\dsa[3].lastCarry ),
    .X(_060_));
 sky130_fd_sc_hd__nand3_1 _136_ (.A(net36),
    .B(net21),
    .C(\dsa[3].lastCarry ),
    .Y(_061_));
 sky130_fd_sc_hd__nand2_1 _137_ (.A(_060_),
    .B(_061_),
    .Y(_062_));
 sky130_fd_sc_hd__xnor2_1 _138_ (.A(net70),
    .B(_062_),
    .Y(\dsa[3].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _139_ (.A1(net40),
    .A2(net27),
    .B1(\dsa[27].lastCarry ),
    .X(_063_));
 sky130_fd_sc_hd__nand3_1 _140_ (.A(net40),
    .B(net27),
    .C(\dsa[27].lastCarry ),
    .Y(_064_));
 sky130_fd_sc_hd__nand2_1 _141_ (.A(_063_),
    .B(_064_),
    .Y(_065_));
 sky130_fd_sc_hd__xnor2_1 _142_ (.A(net57),
    .B(_065_),
    .Y(\dsa[27].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _143_ (.A1(net42),
    .A2(net1),
    .B1(\dsa[31].lastCarry ),
    .X(_066_));
 sky130_fd_sc_hd__nand3_1 _144_ (.A(net42),
    .B(net1),
    .C(\dsa[31].lastCarry ),
    .Y(_067_));
 sky130_fd_sc_hd__nand2_1 _145_ (.A(_066_),
    .B(_067_),
    .Y(_068_));
 sky130_fd_sc_hd__xnor2_1 _146_ (.A(net80),
    .B(_068_),
    .Y(\dsa[31].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _147_ (.A1(net40),
    .A2(net23),
    .B1(\dsa[29].lastCarry ),
    .X(_069_));
 sky130_fd_sc_hd__nand3_1 _148_ (.A(net40),
    .B(net23),
    .C(\dsa[29].lastCarry ),
    .Y(_070_));
 sky130_fd_sc_hd__nand2_1 _149_ (.A(_069_),
    .B(_070_),
    .Y(_071_));
 sky130_fd_sc_hd__xnor2_1 _150_ (.A(net77),
    .B(_071_),
    .Y(\dsa[29].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _151_ (.A1(net38),
    .A2(net13),
    .B1(\dsa[11].lastCarry ),
    .X(_072_));
 sky130_fd_sc_hd__nand3_1 _152_ (.A(net38),
    .B(net13),
    .C(\dsa[11].lastCarry ),
    .Y(_073_));
 sky130_fd_sc_hd__nand2_1 _153_ (.A(_072_),
    .B(_073_),
    .Y(_074_));
 sky130_fd_sc_hd__xnor2_1 _154_ (.A(net66),
    .B(_074_),
    .Y(\dsa[11].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _155_ (.A1(net42),
    .A2(net8),
    .B1(\dsa[15].lastCarry ),
    .X(_075_));
 sky130_fd_sc_hd__nand3_1 _156_ (.A(net42),
    .B(net8),
    .C(\dsa[15].lastCarry ),
    .Y(_076_));
 sky130_fd_sc_hd__nand2_1 _157_ (.A(_075_),
    .B(_076_),
    .Y(_077_));
 sky130_fd_sc_hd__xnor2_1 _158_ (.A(net53),
    .B(_077_),
    .Y(\dsa[15].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _159_ (.A1(net42),
    .A2(net4),
    .B1(\dsa[19].lastCarry ),
    .X(_078_));
 sky130_fd_sc_hd__nand3_1 _160_ (.A(net42),
    .B(net4),
    .C(\dsa[19].lastCarry ),
    .Y(_079_));
 sky130_fd_sc_hd__nand2_1 _161_ (.A(_078_),
    .B(_079_),
    .Y(_080_));
 sky130_fd_sc_hd__xnor2_1 _162_ (.A(net69),
    .B(_080_),
    .Y(\dsa[19].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _163_ (.A1(net41),
    .A2(net32),
    .B1(\dsa[22].lastCarry ),
    .X(_081_));
 sky130_fd_sc_hd__nand3_1 _164_ (.A(net37),
    .B(net32),
    .C(\dsa[22].lastCarry ),
    .Y(_082_));
 sky130_fd_sc_hd__nand2_1 _165_ (.A(_081_),
    .B(_082_),
    .Y(_083_));
 sky130_fd_sc_hd__xnor2_1 _166_ (.A(net84),
    .B(_083_),
    .Y(\dsa[22].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _167_ (.A1(net42),
    .A2(net6),
    .B1(\dsa[17].lastCarry ),
    .X(_084_));
 sky130_fd_sc_hd__nand3_1 _168_ (.A(net42),
    .B(net6),
    .C(\dsa[17].lastCarry ),
    .Y(_085_));
 sky130_fd_sc_hd__nand2_1 _169_ (.A(_084_),
    .B(_085_),
    .Y(_086_));
 sky130_fd_sc_hd__xnor2_1 _170_ (.A(net64),
    .B(_086_),
    .Y(\dsa[17].y_out_next ));
 sky130_fd_sc_hd__and3_1 _171_ (.A(net36),
    .B(net25),
    .C(net81),
    .X(\dsa[0].lastCarry_next ));
 sky130_fd_sc_hd__a21oi_1 _172_ (.A1(net36),
    .A2(net25),
    .B1(net81),
    .Y(_087_));
 sky130_fd_sc_hd__nor2_1 _173_ (.A(\dsa[0].lastCarry_next ),
    .B(_087_),
    .Y(\dsa[0].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _174_ (.A1(net39),
    .A2(net10),
    .B1(\dsa[13].lastCarry ),
    .X(_088_));
 sky130_fd_sc_hd__nand3_1 _175_ (.A(net39),
    .B(net10),
    .C(\dsa[13].lastCarry ),
    .Y(_089_));
 sky130_fd_sc_hd__nand2_1 _176_ (.A(_088_),
    .B(_089_),
    .Y(_090_));
 sky130_fd_sc_hd__xnor2_1 _177_ (.A(net63),
    .B(_090_),
    .Y(\dsa[13].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _178_ (.A1(net41),
    .A2(net31),
    .B1(\dsa[23].lastCarry ),
    .X(_091_));
 sky130_fd_sc_hd__nand3_1 _179_ (.A(net41),
    .B(net31),
    .C(\dsa[23].lastCarry ),
    .Y(_092_));
 sky130_fd_sc_hd__nand2_1 _180_ (.A(_091_),
    .B(_092_),
    .Y(_093_));
 sky130_fd_sc_hd__xnor2_1 _181_ (.A(net58),
    .B(_093_),
    .Y(\dsa[23].y_out_next ));
 sky130_fd_sc_hd__a21bo_1 _182_ (.A1(net78),
    .A2(_000_),
    .B1_N(_001_),
    .X(\dsa[9].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _183_ (.A1(net62),
    .A2(_003_),
    .B1_N(_004_),
    .X(\dsa[2].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _184_ (.A1(net83),
    .A2(_006_),
    .B1_N(_007_),
    .X(\dsa[20].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _185_ (.A1(net55),
    .A2(_009_),
    .B1_N(_010_),
    .X(\dsa[21].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _186_ (.A1(net73),
    .A2(_012_),
    .B1_N(_013_),
    .X(\dsa[1].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _187_ (.A1(net76),
    .A2(_015_),
    .B1_N(_016_),
    .X(\dsa[18].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _188_ (.A1(net60),
    .A2(_018_),
    .B1_N(_019_),
    .X(\dsa[16].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _189_ (.A1(net68),
    .A2(_021_),
    .B1_N(_022_),
    .X(\dsa[14].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _190_ (.A1(net67),
    .A2(_024_),
    .B1_N(_025_),
    .X(\dsa[12].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _191_ (.A1(net75),
    .A2(_027_),
    .B1_N(_028_),
    .X(\dsa[10].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _192_ (.A1(net74),
    .A2(_030_),
    .B1_N(_031_),
    .X(\dsa[24].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _193_ (.A1(net82),
    .A2(_033_),
    .B1_N(_034_),
    .X(\dsa[8].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _194_ (.A1(net54),
    .A2(_036_),
    .B1_N(_037_),
    .X(\dsa[30].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _195_ (.A1(net65),
    .A2(_039_),
    .B1_N(_040_),
    .X(\dsa[7].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _196_ (.A1(net71),
    .A2(_042_),
    .B1_N(_043_),
    .X(\dsa[25].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _197_ (.A1(net72),
    .A2(_045_),
    .B1_N(_046_),
    .X(\dsa[6].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _198_ (.A1(net56),
    .A2(_048_),
    .B1_N(_049_),
    .X(\dsa[28].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _199_ (.A1(net79),
    .A2(_051_),
    .B1_N(_052_),
    .X(\dsa[5].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _200_ (.A1(net59),
    .A2(_054_),
    .B1_N(_055_),
    .X(\dsa[26].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _201_ (.A1(net61),
    .A2(_057_),
    .B1_N(_058_),
    .X(\dsa[4].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _202_ (.A1(net70),
    .A2(_060_),
    .B1_N(_061_),
    .X(\dsa[3].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _203_ (.A1(net57),
    .A2(_063_),
    .B1_N(_064_),
    .X(\dsa[27].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _204_ (.A1(net80),
    .A2(_066_),
    .B1_N(_067_),
    .X(\dsa[31].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _205_ (.A1(net77),
    .A2(_069_),
    .B1_N(_070_),
    .X(\dsa[29].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _206_ (.A1(net66),
    .A2(_072_),
    .B1_N(_073_),
    .X(\dsa[11].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _207_ (.A1(net53),
    .A2(_075_),
    .B1_N(_076_),
    .X(\dsa[15].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _208_ (.A1(net69),
    .A2(_078_),
    .B1_N(_079_),
    .X(\dsa[19].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _209_ (.A1(net84),
    .A2(_081_),
    .B1_N(_082_),
    .X(\dsa[22].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _210_ (.A1(net64),
    .A2(_084_),
    .B1_N(_085_),
    .X(\dsa[17].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _211_ (.A1(net63),
    .A2(_088_),
    .B1_N(_089_),
    .X(\dsa[13].lastCarry_next ));
 sky130_fd_sc_hd__a21bo_1 _212_ (.A1(net58),
    .A2(_091_),
    .B1_N(_092_),
    .X(\dsa[23].lastCarry_next ));
 sky130_fd_sc_hd__a21o_1 _213_ (.A1(net38),
    .A2(net15),
    .B1(\dsa[9].lastCarry ),
    .X(_000_));
 sky130_fd_sc_hd__nand3_1 _214_ (.A(\dsa[9].lastCarry ),
    .B(net38),
    .C(net15),
    .Y(_001_));
 sky130_fd_sc_hd__nand2_1 _215_ (.A(_000_),
    .B(_001_),
    .Y(_002_));
 sky130_fd_sc_hd__xnor2_1 _216_ (.A(net78),
    .B(_002_),
    .Y(\dsa[9].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _217_ (.A1(net22),
    .A2(net36),
    .B1(\dsa[2].lastCarry ),
    .X(_003_));
 sky130_fd_sc_hd__nand3_1 _218_ (.A(net22),
    .B(net36),
    .C(\dsa[2].lastCarry ),
    .Y(_004_));
 sky130_fd_sc_hd__nand2_1 _219_ (.A(_003_),
    .B(_004_),
    .Y(_005_));
 sky130_fd_sc_hd__xnor2_1 _220_ (.A(net62),
    .B(_005_),
    .Y(\dsa[2].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _221_ (.A1(net39),
    .A2(net3),
    .B1(\dsa[20].lastCarry ),
    .X(_006_));
 sky130_fd_sc_hd__nand3_1 _222_ (.A(net39),
    .B(net3),
    .C(\dsa[20].lastCarry ),
    .Y(_007_));
 sky130_fd_sc_hd__nand2_1 _223_ (.A(_006_),
    .B(_007_),
    .Y(_008_));
 sky130_fd_sc_hd__xnor2_1 _224_ (.A(net83),
    .B(_008_),
    .Y(\dsa[20].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _225_ (.A1(net39),
    .A2(net2),
    .B1(\dsa[21].lastCarry ),
    .X(_009_));
 sky130_fd_sc_hd__nand3_1 _226_ (.A(net39),
    .B(net2),
    .C(\dsa[21].lastCarry ),
    .Y(_010_));
 sky130_fd_sc_hd__nand2_1 _227_ (.A(_009_),
    .B(_010_),
    .Y(_011_));
 sky130_fd_sc_hd__xnor2_1 _228_ (.A(net55),
    .B(_011_),
    .Y(\dsa[21].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _229_ (.A1(net37),
    .A2(net24),
    .B1(\dsa[1].lastCarry ),
    .X(_012_));
 sky130_fd_sc_hd__nand3_1 _230_ (.A(net37),
    .B(net24),
    .C(\dsa[1].lastCarry ),
    .Y(_013_));
 sky130_fd_sc_hd__nand2_1 _231_ (.A(_012_),
    .B(_013_),
    .Y(_014_));
 sky130_fd_sc_hd__xnor2_1 _232_ (.A(net73),
    .B(_014_),
    .Y(\dsa[1].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _233_ (.A1(net43),
    .A2(net5),
    .B1(\dsa[18].lastCarry ),
    .X(_015_));
 sky130_fd_sc_hd__nand3_1 _234_ (.A(net43),
    .B(net5),
    .C(\dsa[18].lastCarry ),
    .Y(_016_));
 sky130_fd_sc_hd__nand2_1 _235_ (.A(_015_),
    .B(_016_),
    .Y(_017_));
 sky130_fd_sc_hd__xnor2_1 _236_ (.A(net76),
    .B(_017_),
    .Y(\dsa[18].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _237_ (.A1(net43),
    .A2(net7),
    .B1(\dsa[16].lastCarry ),
    .X(_018_));
 sky130_fd_sc_hd__nand3_1 _238_ (.A(net43),
    .B(net7),
    .C(\dsa[16].lastCarry ),
    .Y(_019_));
 sky130_fd_sc_hd__nand2_1 _239_ (.A(_018_),
    .B(_019_),
    .Y(_020_));
 sky130_fd_sc_hd__xnor2_1 _240_ (.A(net60),
    .B(_020_),
    .Y(\dsa[16].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _241_ (.A1(net42),
    .A2(net9),
    .B1(\dsa[14].lastCarry ),
    .X(_021_));
 sky130_fd_sc_hd__nand3_1 _242_ (.A(net42),
    .B(net9),
    .C(\dsa[14].lastCarry ),
    .Y(_022_));
 sky130_fd_sc_hd__nand2_1 _243_ (.A(_021_),
    .B(_022_),
    .Y(_023_));
 sky130_fd_sc_hd__xnor2_1 _244_ (.A(net68),
    .B(_023_),
    .Y(\dsa[14].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _245_ (.A1(net38),
    .A2(net11),
    .B1(\dsa[12].lastCarry ),
    .X(_024_));
 sky130_fd_sc_hd__nand3_1 _246_ (.A(net38),
    .B(net11),
    .C(\dsa[12].lastCarry ),
    .Y(_025_));
 sky130_fd_sc_hd__nand2_1 _247_ (.A(_024_),
    .B(_025_),
    .Y(_026_));
 sky130_fd_sc_hd__xnor2_1 _248_ (.A(net67),
    .B(_026_),
    .Y(\dsa[12].y_out_next ));
 sky130_fd_sc_hd__a21o_1 _249_ (.A1(net38),
    .A2(net14),
    .B1(\dsa[10].lastCarry ),
    .X(_027_));
 sky130_fd_sc_hd__nand3_1 _250_ (.A(net38),
    .B(net14),
    .C(\dsa[10].lastCarry ),
    .Y(_028_));
 sky130_fd_sc_hd__nand2_1 _251_ (.A(_027_),
    .B(_028_),
    .Y(_029_));
 sky130_fd_sc_hd__dfrtp_1 _252_ (.CLK(clknet_3_0__leaf_clk),
    .D(\dsa[0].lastCarry_next ),
    .RESET_B(net45),
    .Q(\dsa[0].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _253_ (.CLK(clknet_3_0__leaf_clk),
    .D(\dsa[0].y_out_next ),
    .RESET_B(net45),
    .Q(\dsa[0].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _254_ (.CLK(clknet_3_0__leaf_clk),
    .D(\dsa[1].lastCarry_next ),
    .RESET_B(net45),
    .Q(\dsa[1].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _255_ (.CLK(clknet_3_1__leaf_clk),
    .D(\dsa[1].y_out_next ),
    .RESET_B(net44),
    .Q(\dsa[1].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _256_ (.CLK(clknet_3_0__leaf_clk),
    .D(\dsa[2].lastCarry_next ),
    .RESET_B(net44),
    .Q(\dsa[2].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _257_ (.CLK(clknet_3_0__leaf_clk),
    .D(\dsa[2].y_out_next ),
    .RESET_B(net44),
    .Q(\dsa[2].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _258_ (.CLK(clknet_3_0__leaf_clk),
    .D(\dsa[3].lastCarry_next ),
    .RESET_B(net44),
    .Q(\dsa[3].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _259_ (.CLK(clknet_3_0__leaf_clk),
    .D(\dsa[3].y_out_next ),
    .RESET_B(net44),
    .Q(\dsa[3].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _260_ (.CLK(clknet_3_1__leaf_clk),
    .D(\dsa[4].lastCarry_next ),
    .RESET_B(net44),
    .Q(\dsa[4].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _261_ (.CLK(clknet_3_1__leaf_clk),
    .D(\dsa[4].y_out_next ),
    .RESET_B(net44),
    .Q(\dsa[4].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _262_ (.CLK(clknet_3_1__leaf_clk),
    .D(\dsa[5].lastCarry_next ),
    .RESET_B(net44),
    .Q(\dsa[5].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _263_ (.CLK(clknet_3_1__leaf_clk),
    .D(\dsa[5].y_out_next ),
    .RESET_B(net45),
    .Q(\dsa[5].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _264_ (.CLK(clknet_3_0__leaf_clk),
    .D(\dsa[6].lastCarry_next ),
    .RESET_B(net45),
    .Q(\dsa[6].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _265_ (.CLK(clknet_3_1__leaf_clk),
    .D(\dsa[6].y_out_next ),
    .RESET_B(net45),
    .Q(\dsa[6].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _266_ (.CLK(clknet_3_0__leaf_clk),
    .D(\dsa[7].lastCarry_next ),
    .RESET_B(net45),
    .Q(\dsa[7].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _267_ (.CLK(clknet_3_3__leaf_clk),
    .D(\dsa[7].y_out_next ),
    .RESET_B(net45),
    .Q(\dsa[7].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _268_ (.CLK(clknet_3_2__leaf_clk),
    .D(\dsa[8].lastCarry_next ),
    .RESET_B(net48),
    .Q(\dsa[8].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _269_ (.CLK(clknet_3_2__leaf_clk),
    .D(\dsa[8].y_out_next ),
    .RESET_B(net48),
    .Q(\dsa[8].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _270_ (.CLK(clknet_3_2__leaf_clk),
    .D(\dsa[9].lastCarry_next ),
    .RESET_B(net48),
    .Q(\dsa[9].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _271_ (.CLK(clknet_3_3__leaf_clk),
    .D(\dsa[9].y_out_next ),
    .RESET_B(net48),
    .Q(\dsa[10].y_in ));
 sky130_fd_sc_hd__dfrtp_1 _272_ (.CLK(clknet_3_2__leaf_clk),
    .D(\dsa[10].lastCarry_next ),
    .RESET_B(net48),
    .Q(\dsa[10].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _273_ (.CLK(clknet_3_2__leaf_clk),
    .D(\dsa[10].y_out_next ),
    .RESET_B(net48),
    .Q(\dsa[10].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _274_ (.CLK(clknet_3_2__leaf_clk),
    .D(\dsa[11].lastCarry_next ),
    .RESET_B(net49),
    .Q(\dsa[11].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _275_ (.CLK(clknet_3_2__leaf_clk),
    .D(\dsa[11].y_out_next ),
    .RESET_B(net49),
    .Q(\dsa[11].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _276_ (.CLK(clknet_3_2__leaf_clk),
    .D(\dsa[12].lastCarry_next ),
    .RESET_B(net48),
    .Q(\dsa[12].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _277_ (.CLK(clknet_3_3__leaf_clk),
    .D(\dsa[12].y_out_next ),
    .RESET_B(net49),
    .Q(\dsa[12].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _278_ (.CLK(clknet_3_3__leaf_clk),
    .D(\dsa[13].lastCarry_next ),
    .RESET_B(net49),
    .Q(\dsa[13].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _279_ (.CLK(clknet_3_3__leaf_clk),
    .D(\dsa[13].y_out_next ),
    .RESET_B(net49),
    .Q(\dsa[13].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _280_ (.CLK(clknet_3_3__leaf_clk),
    .D(\dsa[14].lastCarry_next ),
    .RESET_B(net50),
    .Q(\dsa[14].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _281_ (.CLK(clknet_3_6__leaf_clk),
    .D(\dsa[14].y_out_next ),
    .RESET_B(net50),
    .Q(\dsa[14].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _282_ (.CLK(clknet_3_6__leaf_clk),
    .D(\dsa[15].lastCarry_next ),
    .RESET_B(net50),
    .Q(\dsa[15].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _283_ (.CLK(clknet_3_7__leaf_clk),
    .D(\dsa[15].y_out_next ),
    .RESET_B(net51),
    .Q(\dsa[15].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _284_ (.CLK(clknet_3_7__leaf_clk),
    .D(\dsa[16].lastCarry_next ),
    .RESET_B(net51),
    .Q(\dsa[16].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _285_ (.CLK(clknet_3_7__leaf_clk),
    .D(\dsa[16].y_out_next ),
    .RESET_B(net51),
    .Q(\dsa[16].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _286_ (.CLK(clknet_3_7__leaf_clk),
    .D(\dsa[17].lastCarry_next ),
    .RESET_B(net51),
    .Q(\dsa[17].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _287_ (.CLK(clknet_3_7__leaf_clk),
    .D(\dsa[17].y_out_next ),
    .RESET_B(net50),
    .Q(\dsa[17].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _288_ (.CLK(clknet_3_6__leaf_clk),
    .D(\dsa[18].lastCarry_next ),
    .RESET_B(net50),
    .Q(\dsa[18].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _289_ (.CLK(clknet_3_7__leaf_clk),
    .D(\dsa[18].y_out_next ),
    .RESET_B(net50),
    .Q(\dsa[18].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _290_ (.CLK(clknet_3_6__leaf_clk),
    .D(\dsa[19].lastCarry_next ),
    .RESET_B(net50),
    .Q(\dsa[19].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _291_ (.CLK(clknet_3_7__leaf_clk),
    .D(\dsa[19].y_out_next ),
    .RESET_B(net50),
    .Q(\dsa[19].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _292_ (.CLK(clknet_3_3__leaf_clk),
    .D(\dsa[20].lastCarry_next ),
    .RESET_B(net49),
    .Q(\dsa[20].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _293_ (.CLK(clknet_3_6__leaf_clk),
    .D(\dsa[20].y_out_next ),
    .RESET_B(net48),
    .Q(\dsa[20].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _294_ (.CLK(clknet_3_3__leaf_clk),
    .D(\dsa[21].lastCarry_next ),
    .RESET_B(net48),
    .Q(\dsa[21].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _295_ (.CLK(clknet_3_3__leaf_clk),
    .D(\dsa[21].y_out_next ),
    .RESET_B(net48),
    .Q(\dsa[21].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _296_ (.CLK(clknet_3_1__leaf_clk),
    .D(\dsa[22].lastCarry_next ),
    .RESET_B(net45),
    .Q(\dsa[22].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _297_ (.CLK(clknet_3_4__leaf_clk),
    .D(\dsa[22].y_out_next ),
    .RESET_B(net46),
    .Q(\dsa[22].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _298_ (.CLK(clknet_3_4__leaf_clk),
    .D(\dsa[23].lastCarry_next ),
    .RESET_B(net47),
    .Q(\dsa[23].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _299_ (.CLK(clknet_3_4__leaf_clk),
    .D(\dsa[23].y_out_next ),
    .RESET_B(net46),
    .Q(\dsa[23].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _300_ (.CLK(clknet_3_5__leaf_clk),
    .D(\dsa[24].lastCarry_next ),
    .RESET_B(net46),
    .Q(\dsa[24].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _301_ (.CLK(clknet_3_1__leaf_clk),
    .D(\dsa[24].y_out_next ),
    .RESET_B(net44),
    .Q(\dsa[24].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _302_ (.CLK(clknet_3_1__leaf_clk),
    .D(\dsa[25].lastCarry_next ),
    .RESET_B(net44),
    .Q(\dsa[25].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _303_ (.CLK(clknet_3_4__leaf_clk),
    .D(\dsa[25].y_out_next ),
    .RESET_B(net46),
    .Q(\dsa[25].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _304_ (.CLK(clknet_3_4__leaf_clk),
    .D(\dsa[26].lastCarry_next ),
    .RESET_B(net46),
    .Q(\dsa[26].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _305_ (.CLK(clknet_3_4__leaf_clk),
    .D(\dsa[26].y_out_next ),
    .RESET_B(net46),
    .Q(\dsa[26].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _306_ (.CLK(clknet_3_4__leaf_clk),
    .D(\dsa[27].lastCarry_next ),
    .RESET_B(net46),
    .Q(\dsa[27].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _307_ (.CLK(clknet_3_5__leaf_clk),
    .D(\dsa[27].y_out_next ),
    .RESET_B(net46),
    .Q(\dsa[27].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _308_ (.CLK(clknet_3_5__leaf_clk),
    .D(\dsa[28].lastCarry_next ),
    .RESET_B(net46),
    .Q(\dsa[28].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _309_ (.CLK(clknet_3_5__leaf_clk),
    .D(\dsa[28].y_out_next ),
    .RESET_B(net46),
    .Q(\dsa[28].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _310_ (.CLK(clknet_3_5__leaf_clk),
    .D(\dsa[29].lastCarry_next ),
    .RESET_B(net47),
    .Q(\dsa[29].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _311_ (.CLK(clknet_3_5__leaf_clk),
    .D(\dsa[29].y_out_next ),
    .RESET_B(net47),
    .Q(\dsa[29].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _312_ (.CLK(clknet_3_6__leaf_clk),
    .D(\dsa[30].lastCarry_next ),
    .RESET_B(net47),
    .Q(\dsa[30].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _313_ (.CLK(clknet_3_7__leaf_clk),
    .D(\dsa[30].y_out_next ),
    .RESET_B(net50),
    .Q(\dsa[30].y_out ));
 sky130_fd_sc_hd__dfrtp_1 _314_ (.CLK(clknet_3_7__leaf_clk),
    .D(\dsa[31].lastCarry_next ),
    .RESET_B(net50),
    .Q(\dsa[31].lastCarry ));
 sky130_fd_sc_hd__dfrtp_1 _315_ (.CLK(clknet_3_5__leaf_clk),
    .D(\dsa[31].y_out_next ),
    .RESET_B(net47),
    .Q(net35));
 sky130_fd_sc_hd__clkbuf_1 input1 (.A(a[0]),
    .X(net1));
 sky130_fd_sc_hd__buf_1 input2 (.A(a[10]),
    .X(net2));
 sky130_fd_sc_hd__clkbuf_1 input3 (.A(a[11]),
    .X(net3));
 sky130_fd_sc_hd__buf_1 input4 (.A(a[12]),
    .X(net4));
 sky130_fd_sc_hd__clkbuf_1 input5 (.A(a[13]),
    .X(net5));
 sky130_fd_sc_hd__clkbuf_1 input6 (.A(a[14]),
    .X(net6));
 sky130_fd_sc_hd__clkbuf_1 input7 (.A(a[15]),
    .X(net7));
 sky130_fd_sc_hd__clkbuf_1 input8 (.A(a[16]),
    .X(net8));
 sky130_fd_sc_hd__clkbuf_1 input9 (.A(a[17]),
    .X(net9));
 sky130_fd_sc_hd__clkbuf_1 input10 (.A(a[18]),
    .X(net10));
 sky130_fd_sc_hd__clkbuf_1 input11 (.A(a[19]),
    .X(net11));
 sky130_fd_sc_hd__clkbuf_1 input12 (.A(a[1]),
    .X(net12));
 sky130_fd_sc_hd__clkbuf_1 input13 (.A(a[20]),
    .X(net13));
 sky130_fd_sc_hd__clkbuf_1 input14 (.A(a[21]),
    .X(net14));
 sky130_fd_sc_hd__clkbuf_1 input15 (.A(a[22]),
    .X(net15));
 sky130_fd_sc_hd__clkbuf_1 input16 (.A(a[23]),
    .X(net16));
 sky130_fd_sc_hd__clkbuf_1 input17 (.A(a[24]),
    .X(net17));
 sky130_fd_sc_hd__clkbuf_1 input18 (.A(a[25]),
    .X(net18));
 sky130_fd_sc_hd__clkbuf_1 input19 (.A(a[26]),
    .X(net19));
 sky130_fd_sc_hd__clkbuf_1 input20 (.A(a[27]),
    .X(net20));
 sky130_fd_sc_hd__clkbuf_1 input21 (.A(a[28]),
    .X(net21));
 sky130_fd_sc_hd__clkbuf_1 input22 (.A(a[29]),
    .X(net22));
 sky130_fd_sc_hd__clkbuf_1 input23 (.A(a[2]),
    .X(net23));
 sky130_fd_sc_hd__clkbuf_1 input24 (.A(a[30]),
    .X(net24));
 sky130_fd_sc_hd__clkbuf_1 input25 (.A(a[31]),
    .X(net25));
 sky130_fd_sc_hd__clkbuf_1 input26 (.A(a[3]),
    .X(net26));
 sky130_fd_sc_hd__clkbuf_1 input27 (.A(a[4]),
    .X(net27));
 sky130_fd_sc_hd__clkbuf_1 input28 (.A(a[5]),
    .X(net28));
 sky130_fd_sc_hd__clkbuf_1 input29 (.A(a[6]),
    .X(net29));
 sky130_fd_sc_hd__clkbuf_1 input30 (.A(a[7]),
    .X(net30));
 sky130_fd_sc_hd__buf_1 input31 (.A(a[8]),
    .X(net31));
 sky130_fd_sc_hd__buf_1 input32 (.A(a[9]),
    .X(net32));
 sky130_fd_sc_hd__clkbuf_1 input33 (.A(rst),
    .X(net33));
 sky130_fd_sc_hd__dlymetal6s2s_1 input34 (.A(x),
    .X(net34));
 sky130_fd_sc_hd__buf_2 output35 (.A(net35),
    .X(y));
 sky130_fd_sc_hd__buf_2 fanout36 (.A(net37),
    .X(net36));
 sky130_fd_sc_hd__clkbuf_2 fanout37 (.A(net34),
    .X(net37));
 sky130_fd_sc_hd__buf_2 fanout38 (.A(net34),
    .X(net38));
 sky130_fd_sc_hd__clkbuf_2 fanout39 (.A(net34),
    .X(net39));
 sky130_fd_sc_hd__buf_2 fanout40 (.A(net41),
    .X(net40));
 sky130_fd_sc_hd__clkbuf_2 fanout41 (.A(net43),
    .X(net41));
 sky130_fd_sc_hd__buf_2 fanout42 (.A(net43),
    .X(net42));
 sky130_fd_sc_hd__clkbuf_2 fanout43 (.A(net34),
    .X(net43));
 sky130_fd_sc_hd__clkbuf_4 fanout44 (.A(net52),
    .X(net44));
 sky130_fd_sc_hd__clkbuf_4 fanout45 (.A(net52),
    .X(net45));
 sky130_fd_sc_hd__clkbuf_4 fanout46 (.A(net52),
    .X(net46));
 sky130_fd_sc_hd__clkbuf_2 fanout47 (.A(net52),
    .X(net47));
 sky130_fd_sc_hd__clkbuf_4 fanout48 (.A(net51),
    .X(net48));
 sky130_fd_sc_hd__clkbuf_2 fanout49 (.A(net51),
    .X(net49));
 sky130_fd_sc_hd__clkbuf_4 fanout50 (.A(net51),
    .X(net50));
 sky130_fd_sc_hd__buf_2 fanout51 (.A(net52),
    .X(net51));
 sky130_fd_sc_hd__clkbuf_2 fanout52 (.A(net33),
    .X(net52));
 sky130_fd_sc_hd__clkbuf_16 clkbuf_0_clk (.A(clk),
    .X(clknet_0_clk));
 sky130_fd_sc_hd__clkbuf_16 clkbuf_3_0__f_clk (.A(clknet_0_clk),
    .X(clknet_3_0__leaf_clk));
 sky130_fd_sc_hd__clkbuf_16 clkbuf_3_1__f_clk (.A(clknet_0_clk),
    .X(clknet_3_1__leaf_clk));
 sky130_fd_sc_hd__clkbuf_16 clkbuf_3_2__f_clk (.A(clknet_0_clk),
    .X(clknet_3_2__leaf_clk));
 sky130_fd_sc_hd__clkbuf_16 clkbuf_3_3__f_clk (.A(clknet_0_clk),
    .X(clknet_3_3__leaf_clk));
 sky130_fd_sc_hd__clkbuf_16 clkbuf_3_4__f_clk (.A(clknet_0_clk),
    .X(clknet_3_4__leaf_clk));
 sky130_fd_sc_hd__clkbuf_16 clkbuf_3_5__f_clk (.A(clknet_0_clk),
    .X(clknet_3_5__leaf_clk));
 sky130_fd_sc_hd__clkbuf_16 clkbuf_3_6__f_clk (.A(clknet_0_clk),
    .X(clknet_3_6__leaf_clk));
 sky130_fd_sc_hd__clkbuf_16 clkbuf_3_7__f_clk (.A(clknet_0_clk),
    .X(clknet_3_7__leaf_clk));
 sky130_fd_sc_hd__dlygate4sd3_1 hold1 (.A(\dsa[14].y_out ),
    .X(net53));
 sky130_fd_sc_hd__dlygate4sd3_1 hold2 (.A(\dsa[29].y_out ),
    .X(net54));
 sky130_fd_sc_hd__dlygate4sd3_1 hold3 (.A(\dsa[20].y_out ),
    .X(net55));
 sky130_fd_sc_hd__dlygate4sd3_1 hold4 (.A(\dsa[27].y_out ),
    .X(net56));
 sky130_fd_sc_hd__dlygate4sd3_1 hold5 (.A(\dsa[26].y_out ),
    .X(net57));
 sky130_fd_sc_hd__dlygate4sd3_1 hold6 (.A(\dsa[22].y_out ),
    .X(net58));
 sky130_fd_sc_hd__dlygate4sd3_1 hold7 (.A(\dsa[25].y_out ),
    .X(net59));
 sky130_fd_sc_hd__dlygate4sd3_1 hold8 (.A(\dsa[15].y_out ),
    .X(net60));
 sky130_fd_sc_hd__dlygate4sd3_1 hold9 (.A(\dsa[3].y_out ),
    .X(net61));
 sky130_fd_sc_hd__dlygate4sd3_1 hold10 (.A(\dsa[1].y_out ),
    .X(net62));
 sky130_fd_sc_hd__dlygate4sd3_1 hold11 (.A(\dsa[12].y_out ),
    .X(net63));
 sky130_fd_sc_hd__dlygate4sd3_1 hold12 (.A(\dsa[16].y_out ),
    .X(net64));
 sky130_fd_sc_hd__dlygate4sd3_1 hold13 (.A(\dsa[6].y_out ),
    .X(net65));
 sky130_fd_sc_hd__dlygate4sd3_1 hold14 (.A(\dsa[10].y_out ),
    .X(net66));
 sky130_fd_sc_hd__dlygate4sd3_1 hold15 (.A(\dsa[11].y_out ),
    .X(net67));
 sky130_fd_sc_hd__dlygate4sd3_1 hold16 (.A(\dsa[13].y_out ),
    .X(net68));
 sky130_fd_sc_hd__dlygate4sd3_1 hold17 (.A(\dsa[18].y_out ),
    .X(net69));
 sky130_fd_sc_hd__dlygate4sd3_1 hold18 (.A(\dsa[2].y_out ),
    .X(net70));
 sky130_fd_sc_hd__dlygate4sd3_1 hold19 (.A(\dsa[24].y_out ),
    .X(net71));
 sky130_fd_sc_hd__dlygate4sd3_1 hold20 (.A(\dsa[5].y_out ),
    .X(net72));
 sky130_fd_sc_hd__dlygate4sd3_1 hold21 (.A(\dsa[0].y_out ),
    .X(net73));
 sky130_fd_sc_hd__dlygate4sd3_1 hold22 (.A(\dsa[23].y_out ),
    .X(net74));
 sky130_fd_sc_hd__dlygate4sd3_1 hold23 (.A(\dsa[10].y_in ),
    .X(net75));
 sky130_fd_sc_hd__dlygate4sd3_1 hold24 (.A(\dsa[17].y_out ),
    .X(net76));
 sky130_fd_sc_hd__dlygate4sd3_1 hold25 (.A(\dsa[28].y_out ),
    .X(net77));
 sky130_fd_sc_hd__dlygate4sd3_1 hold26 (.A(\dsa[8].y_out ),
    .X(net78));
 sky130_fd_sc_hd__dlygate4sd3_1 hold27 (.A(\dsa[4].y_out ),
    .X(net79));
 sky130_fd_sc_hd__dlygate4sd3_1 hold28 (.A(\dsa[30].y_out ),
    .X(net80));
 sky130_fd_sc_hd__dlygate4sd3_1 hold29 (.A(\dsa[0].lastCarry ),
    .X(net81));
 sky130_fd_sc_hd__dlygate4sd3_1 hold30 (.A(\dsa[7].y_out ),
    .X(net82));
 sky130_fd_sc_hd__dlygate4sd3_1 hold31 (.A(\dsa[19].y_out ),
    .X(net83));
 sky130_fd_sc_hd__dlygate4sd3_1 hold32 (.A(\dsa[21].y_out ),
    .X(net84));
endmodule
