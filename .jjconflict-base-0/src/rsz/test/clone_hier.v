module hi_fanout (clk1,
    data,
    output0,
    output1,
    output2,
    output3,
    output4,
    output5,
    output6,
    output7,
    output8,
    output9,
    output10,
    output11,
    output12,
    output13,
    output14,
    output15,
    output16,
    output17,
    output18,
    output19,
    output20,
    output21,
    output22,
    output23,
    output24,
    output25,
    output26,
    output27,
    output28,
    output29,
    output30,
    output31,
    output32,
    output33,
    output34,
    output35,
    output36,
    output37,
    output38,
    output39,
    output40,
    output41,
    output42,
    output43,
    output44,
    output45,
    output46,
    output47,
    output48,
    output49,
    output50,
    output51,
    output52,
    output53,
    output54,
    output55,
    output56,
    output57,
    output58,
    output59,
    output60,
    output61,
    output62,
    output63,
    output64,
    output65,
    output66,
    output67,
    output68,
    output69,
    output70,
    output71,
    output72,
    output73,
    output74,
    output75,
    output76,
    output77,
    output78,
    output79,
    output80,
    output81,
    output82,
    output83,
    output84,
    output85,
    output86,
    output87,
    output88,
    output89,
    output90,
    output91,
    output92,
    output93,
    output94,
    output95,
    output96,
    output97,
    output98,
    output99,
    output100,
    output101,
    output102,
    output103,
    output104,
    output105,
    output106,
    output107,
    output108,
    output109,
    output110,
    output111,
    output112,
    output113,
    output114,
    output115,
    output116,
    output117,
    output118,
    output119,
    output120,
    output121,
    output122,
    output123,
    output124,
    output125,
    output126,
    output127,
    output128,
    output129,
    output130,
    output131,
    output132,
    output133,
    output134,
    output135,
    output136,
    output137,
    output138,
    output139,
    output140,
    output141,
    output142,
    output143,
    output144,
    output145,
    output146,
    output147,
    output148,
    output149);
 input clk1;
 input data;
 output output0;
 output output1;
 output output2;
 output output3;
 output output4;
 output output5;
 output output6;
 output output7;
 output output8;
 output output9;
 output output10;
 output output11;
 output output12;
 output output13;
 output output14;
 output output15;
 output output16;
 output output17;
 output output18;
 output output19;
 output output20;
 output output21;
 output output22;
 output output23;
 output output24;
 output output25;
 output output26;
 output output27;
 output output28;
 output output29;
 output output30;
 output output31;
 output output32;
 output output33;
 output output34;
 output output35;
 output output36;
 output output37;
 output output38;
 output output39;
 output output40;
 output output41;
 output output42;
 output output43;
 output output44;
 output output45;
 output output46;
 output output47;
 output output48;
 output output49;
 output output50;
 output output51;
 output output52;
 output output53;
 output output54;
 output output55;
 output output56;
 output output57;
 output output58;
 output output59;
 output output60;
 output output61;
 output output62;
 output output63;
 output output64;
 output output65;
 output output66;
 output output67;
 output output68;
 output output69;
 output output70;
 output output71;
 output output72;
 output output73;
 output output74;
 output output75;
 output output76;
 output output77;
 output output78;
 output output79;
 output output80;
 output output81;
 output output82;
 output output83;
 output output84;
 output output85;
 output output86;
 output output87;
 output output88;
 output output89;
 output output90;
 output output91;
 output output92;
 output output93;
 output output94;
 output output95;
 output output96;
 output output97;
 output output98;
 output output99;
 output output100;
 output output101;
 output output102;
 output output103;
 output output104;
 output output105;
 output output106;
 output output107;
 output output108;
 output output109;
 output output110;
 output output111;
 output output112;
 output output113;
 output output114;
 output output115;
 output output116;
 output output117;
 output output118;
 output output119;
 output output120;
 output output121;
 output output122;
 output output123;
 output output124;
 output output125;
 output output126;
 output output127;
 output output128;
 output output129;
 output output130;
 output output131;
 output output132;
 output output133;
 output output134;
 output output135;
 output output136;
 output output137;
 output output138;
 output output139;
 output output140;
 output output141;
 output output142;
 output output143;
 output output144;
 output output145;
 output output146;
 output output147;
 output output148;
 output output149;

 wire clk_to_nand0;
 wire clk_to_nand1;
 wire net0;

 DFF_X1 drvr_1 (.D(data),
    .CK(clk1),
    .Q(clk_to_nand0));
 DFF_X1 drvr_2 (.D(data),
    .CK(clk1),
    .Q(clk_to_nand1));

   submodule cloneU1 (.ip0(clk_to_nand0),
		      .ip1(clk_to_nand1),
		      .op0(net0));
   

 DFF_X1 load0 (.D(net0),
    .CK(clk1),
    .Q(output0));
 DFF_X1 load1 (.D(net0),
    .CK(clk1),
    .Q(output1));
 DFF_X1 load2 (.D(net0),
    .CK(clk1),
    .Q(output2));
 DFF_X1 load3 (.D(net0),
    .CK(clk1),
    .Q(output3));
 DFF_X1 load4 (.D(net0),
    .CK(clk1),
    .Q(output4));
 DFF_X1 load5 (.D(net0),
    .CK(clk1),
    .Q(output5));
 DFF_X1 load6 (.D(net0),
    .CK(clk1),
    .Q(output6));
 DFF_X1 load7 (.D(net0),
    .CK(clk1),
    .Q(output7));
 DFF_X1 load8 (.D(net0),
    .CK(clk1),
    .Q(output8));
 DFF_X1 load9 (.D(net0),
    .CK(clk1),
    .Q(output9));
 DFF_X1 load10 (.D(net0),
    .CK(clk1),
    .Q(output10));
 DFF_X1 load11 (.D(net0),
    .CK(clk1),
    .Q(output11));
 DFF_X1 load12 (.D(net0),
    .CK(clk1),
    .Q(output12));
 DFF_X1 load13 (.D(net0),
    .CK(clk1),
    .Q(output13));
 DFF_X1 load14 (.D(net0),
    .CK(clk1),
    .Q(output14));
 DFF_X1 load15 (.D(net0),
    .CK(clk1),
    .Q(output15));
 DFF_X1 load16 (.D(net0),
    .CK(clk1),
    .Q(output16));
 DFF_X1 load17 (.D(net0),
    .CK(clk1),
    .Q(output17));
 DFF_X1 load18 (.D(net0),
    .CK(clk1),
    .Q(output18));
 DFF_X1 load19 (.D(net0),
    .CK(clk1),
    .Q(output19));
 DFF_X1 load20 (.D(net0),
    .CK(clk1),
    .Q(output20));
 DFF_X1 load21 (.D(net0),
    .CK(clk1),
    .Q(output21));
 DFF_X1 load22 (.D(net0),
    .CK(clk1),
    .Q(output22));
 DFF_X1 load23 (.D(net0),
    .CK(clk1),
    .Q(output23));
 DFF_X1 load24 (.D(net0),
    .CK(clk1),
    .Q(output24));
 DFF_X1 load25 (.D(net0),
    .CK(clk1),
    .Q(output25));
 DFF_X1 load26 (.D(net0),
    .CK(clk1),
    .Q(output26));
 DFF_X1 load27 (.D(net0),
    .CK(clk1),
    .Q(output27));
 DFF_X1 load28 (.D(net0),
    .CK(clk1),
    .Q(output28));
 DFF_X1 load29 (.D(net0),
    .CK(clk1),
    .Q(output29));
 DFF_X1 load30 (.D(net0),
    .CK(clk1),
    .Q(output30));
 DFF_X1 load31 (.D(net0),
    .CK(clk1),
    .Q(output31));
 DFF_X1 load32 (.D(net0),
    .CK(clk1),
    .Q(output32));
 DFF_X1 load33 (.D(net0),
    .CK(clk1),
    .Q(output33));
 DFF_X1 load34 (.D(net0),
    .CK(clk1),
    .Q(output34));
 DFF_X1 load35 (.D(net0),
    .CK(clk1),
    .Q(output35));
 DFF_X1 load36 (.D(net0),
    .CK(clk1),
    .Q(output36));
 DFF_X1 load37 (.D(net0),
    .CK(clk1),
    .Q(output37));
 DFF_X1 load38 (.D(net0),
    .CK(clk1),
    .Q(output38));
 DFF_X1 load39 (.D(net0),
    .CK(clk1),
    .Q(output39));
 DFF_X1 load40 (.D(net0),
    .CK(clk1),
    .Q(output40));
 DFF_X1 load41 (.D(net0),
    .CK(clk1),
    .Q(output41));
 DFF_X1 load42 (.D(net0),
    .CK(clk1),
    .Q(output42));
 DFF_X1 load43 (.D(net0),
    .CK(clk1),
    .Q(output43));
 DFF_X1 load44 (.D(net0),
    .CK(clk1),
    .Q(output44));
 DFF_X1 load45 (.D(net0),
    .CK(clk1),
    .Q(output45));
 DFF_X1 load46 (.D(net0),
    .CK(clk1),
    .Q(output46));
 DFF_X1 load47 (.D(net0),
    .CK(clk1),
    .Q(output47));
 DFF_X1 load48 (.D(net0),
    .CK(clk1),
    .Q(output48));
 DFF_X1 load49 (.D(net0),
    .CK(clk1),
    .Q(output49));
 DFF_X1 load50 (.D(net0),
    .CK(clk1),
    .Q(output50));
 DFF_X1 load51 (.D(net0),
    .CK(clk1),
    .Q(output51));
 DFF_X1 load52 (.D(net0),
    .CK(clk1),
    .Q(output52));
 DFF_X1 load53 (.D(net0),
    .CK(clk1),
    .Q(output53));
 DFF_X1 load54 (.D(net0),
    .CK(clk1),
    .Q(output54));
 DFF_X1 load55 (.D(net0),
    .CK(clk1),
    .Q(output55));
 DFF_X1 load56 (.D(net0),
    .CK(clk1),
    .Q(output56));
 DFF_X1 load57 (.D(net0),
    .CK(clk1),
    .Q(output57));
 DFF_X1 load58 (.D(net0),
    .CK(clk1),
    .Q(output58));
 DFF_X1 load59 (.D(net0),
    .CK(clk1),
    .Q(output59));
 DFF_X1 load60 (.D(net0),
    .CK(clk1),
    .Q(output60));
 DFF_X1 load61 (.D(net0),
    .CK(clk1),
    .Q(output61));
 DFF_X1 load62 (.D(net0),
    .CK(clk1),
    .Q(output62));
 DFF_X1 load63 (.D(net0),
    .CK(clk1),
    .Q(output63));
 DFF_X1 load64 (.D(net0),
    .CK(clk1),
    .Q(output64));
 DFF_X1 load65 (.D(net0),
    .CK(clk1),
    .Q(output65));
 DFF_X1 load66 (.D(net0),
    .CK(clk1),
    .Q(output66));
 DFF_X1 load67 (.D(net0),
    .CK(clk1),
    .Q(output67));
 DFF_X1 load68 (.D(net0),
    .CK(clk1),
    .Q(output68));
 DFF_X1 load69 (.D(net0),
    .CK(clk1),
    .Q(output69));
 DFF_X1 load70 (.D(net0),
    .CK(clk1),
    .Q(output70));
 DFF_X1 load71 (.D(net0),
    .CK(clk1),
    .Q(output71));
 DFF_X1 load72 (.D(net0),
    .CK(clk1),
    .Q(output72));
 DFF_X1 load73 (.D(net0),
    .CK(clk1),
    .Q(output73));
 DFF_X1 load74 (.D(net0),
    .CK(clk1),
    .Q(output74));
 DFF_X1 load75 (.D(net0),
    .CK(clk1),
    .Q(output75));
 DFF_X1 load76 (.D(net0),
    .CK(clk1),
    .Q(output76));
 DFF_X1 load77 (.D(net0),
    .CK(clk1),
    .Q(output77));
 DFF_X1 load78 (.D(net0),
    .CK(clk1),
    .Q(output78));
 DFF_X1 load79 (.D(net0),
    .CK(clk1),
    .Q(output79));
 DFF_X1 load80 (.D(net0),
    .CK(clk1),
    .Q(output80));
 DFF_X1 load81 (.D(net0),
    .CK(clk1),
    .Q(output81));
 DFF_X1 load82 (.D(net0),
    .CK(clk1),
    .Q(output82));
 DFF_X1 load83 (.D(net0),
    .CK(clk1),
    .Q(output83));
 DFF_X1 load84 (.D(net0),
    .CK(clk1),
    .Q(output84));
 DFF_X1 load85 (.D(net0),
    .CK(clk1),
    .Q(output85));
 DFF_X1 load86 (.D(net0),
    .CK(clk1),
    .Q(output86));
 DFF_X1 load87 (.D(net0),
    .CK(clk1),
    .Q(output87));
 DFF_X1 load88 (.D(net0),
    .CK(clk1),
    .Q(output88));
 DFF_X1 load89 (.D(net0),
    .CK(clk1),
    .Q(output89));
 DFF_X1 load90 (.D(net0),
    .CK(clk1),
    .Q(output90));
 DFF_X1 load91 (.D(net0),
    .CK(clk1),
    .Q(output91));
 DFF_X1 load92 (.D(net0),
    .CK(clk1),
    .Q(output92));
 DFF_X1 load93 (.D(net0),
    .CK(clk1),
    .Q(output93));
 DFF_X1 load94 (.D(net0),
    .CK(clk1),
    .Q(output94));
 DFF_X1 load95 (.D(net0),
    .CK(clk1),
    .Q(output95));
 DFF_X1 load96 (.D(net0),
    .CK(clk1),
    .Q(output96));
 DFF_X1 load97 (.D(net0),
    .CK(clk1),
    .Q(output97));
 DFF_X1 load98 (.D(net0),
    .CK(clk1),
    .Q(output98));
 DFF_X1 load99 (.D(net0),
    .CK(clk1),
    .Q(output99));
 DFF_X1 load100 (.D(net0),
    .CK(clk1),
    .Q(output100));
 DFF_X1 load101 (.D(net0),
    .CK(clk1),
    .Q(output101));
 DFF_X1 load102 (.D(net0),
    .CK(clk1),
    .Q(output102));
 DFF_X1 load103 (.D(net0),
    .CK(clk1),
    .Q(output103));
 DFF_X1 load104 (.D(net0),
    .CK(clk1),
    .Q(output104));
 DFF_X1 load105 (.D(net0),
    .CK(clk1),
    .Q(output105));
 DFF_X1 load106 (.D(net0),
    .CK(clk1),
    .Q(output106));
 DFF_X1 load107 (.D(net0),
    .CK(clk1),
    .Q(output107));
 DFF_X1 load108 (.D(net0),
    .CK(clk1),
    .Q(output108));
 DFF_X1 load109 (.D(net0),
    .CK(clk1),
    .Q(output109));
 DFF_X1 load110 (.D(net0),
    .CK(clk1),
    .Q(output110));
 DFF_X1 load111 (.D(net0),
    .CK(clk1),
    .Q(output111));
 DFF_X1 load112 (.D(net0),
    .CK(clk1),
    .Q(output112));
 DFF_X1 load113 (.D(net0),
    .CK(clk1),
    .Q(output113));
 DFF_X1 load114 (.D(net0),
    .CK(clk1),
    .Q(output114));
 DFF_X1 load115 (.D(net0),
    .CK(clk1),
    .Q(output115));
 DFF_X1 load116 (.D(net0),
    .CK(clk1),
    .Q(output116));
 DFF_X1 load117 (.D(net0),
    .CK(clk1),
    .Q(output117));
 DFF_X1 load118 (.D(net0),
    .CK(clk1),
    .Q(output118));
 DFF_X1 load119 (.D(net0),
    .CK(clk1),
    .Q(output119));
 DFF_X1 load120 (.D(net0),
    .CK(clk1),
    .Q(output120));
 DFF_X1 load121 (.D(net0),
    .CK(clk1),
    .Q(output121));
 DFF_X1 load122 (.D(net0),
    .CK(clk1),
    .Q(output122));
 DFF_X1 load123 (.D(net0),
    .CK(clk1),
    .Q(output123));
 DFF_X1 load124 (.D(net0),
    .CK(clk1),
    .Q(output124));
 DFF_X1 load125 (.D(net0),
    .CK(clk1),
    .Q(output125));
 DFF_X1 load126 (.D(net0),
    .CK(clk1),
    .Q(output126));
 DFF_X1 load127 (.D(net0),
    .CK(clk1),
    .Q(output127));
 DFF_X1 load128 (.D(net0),
    .CK(clk1),
    .Q(output128));
 DFF_X1 load129 (.D(net0),
    .CK(clk1),
    .Q(output129));
 DFF_X1 load130 (.D(net0),
    .CK(clk1),
    .Q(output130));
 DFF_X1 load131 (.D(net0),
    .CK(clk1),
    .Q(output131));
 DFF_X1 load132 (.D(net0),
    .CK(clk1),
    .Q(output132));
 DFF_X1 load133 (.D(net0),
    .CK(clk1),
    .Q(output133));
 DFF_X1 load134 (.D(net0),
    .CK(clk1),
    .Q(output134));
 DFF_X1 load135 (.D(net0),
    .CK(clk1),
    .Q(output135));
 DFF_X1 load136 (.D(net0),
    .CK(clk1),
    .Q(output136));
 DFF_X1 load137 (.D(net0),
    .CK(clk1),
    .Q(output137));
 DFF_X1 load138 (.D(net0),
    .CK(clk1),
    .Q(output138));
 DFF_X1 load139 (.D(net0),
    .CK(clk1),
    .Q(output139));
 DFF_X1 load140 (.D(net0),
    .CK(clk1),
    .Q(output140));
 DFF_X1 load141 (.D(net0),
    .CK(clk1),
    .Q(output141));
 DFF_X1 load142 (.D(net0),
    .CK(clk1),
    .Q(output142));
 DFF_X1 load143 (.D(net0),
    .CK(clk1),
    .Q(output143));
 DFF_X1 load144 (.D(net0),
    .CK(clk1),
    .Q(output144));
 DFF_X1 load145 (.D(net0),
    .CK(clk1),
    .Q(output145));
 DFF_X1 load146 (.D(net0),
    .CK(clk1),
    .Q(output146));
 DFF_X1 load147 (.D(net0),
    .CK(clk1),
    .Q(output147));
 DFF_X1 load148 (.D(net0),
    .CK(clk1),
    .Q(output148));
 DFF_X1 load149 (.D(net0),
    .CK(clk1),
    .Q(output149));
endmodule


module submodule(ip0,
	       ip1,
	       op0
	       );
   input ip0;
   input ip1;
   output op0;
   
   
 NAND2_X4 nand_inst_0 (.A1(ip0),
		       .A2(ip1),
		       .ZN(op0));
endmodule // cloneU1
