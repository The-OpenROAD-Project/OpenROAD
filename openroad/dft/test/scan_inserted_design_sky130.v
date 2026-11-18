module scan_inserted_design (clock1,
    clock2,
    output1,
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
    output2,
    output20,
    output3,
    output4,
    output5,
    output6,
    output7,
    output8,
    output9,
    port1,
    set_b,
    scan_enable_1,
    scan_in_1,
    scan_in_2,
    scan_in_3,
    scan_in_4,
    selector1,
    alternative_port1);
 input clock1;
 input clock2;
 output output1;
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
 output output2;
 output output20;
 output output3;
 output output4;
 output output5;
 output output6;
 output output7;
 output output8;
 output output9;
 input port1;
 input set_b;
 input scan_enable_1;
 input scan_in_1;
 input scan_in_2;
 input scan_in_3;
 input scan_in_4;
 input selector1;
 input alternative_port1;

 wire mux_out1;
 wire isolated_wire;

 sky130_fd_sc_hd__mux2_1 mux1(.A0(port1), .A1(alternative_port1), .S(selector1), .X(mux_out1));

 sky130_fd_sc_hd__sdfsbp_1 ff1_clk1_rising (.D(port1),
    .Q(output1),
    .SCD(mux_out1),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK(clock1));
 sky130_fd_sc_hd__sdfbbn_1 ff2_clk2_falling (.D(port1),
    .Q(output12),
    .SCD(output14),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK_N(clock2));
 sky130_fd_sc_hd__sdfsbp_1 ff2_clk2_rising (.D(port1),
    .Q(output2),
    .SCD(output4),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK(clock2));

 sky130_fd_sc_hd__lpflow_inputiso0n_1 iso(.A(output2), .X(isolated_wire));

 sky130_fd_sc_hd__sdfbbn_1 ff3_clk1_falling (.D(port1),
    .Q(output13),
    .SCD(output15),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK_N(clock1));
 sky130_fd_sc_hd__sdfsbp_1 ff3_clk1_rising (.D(port1),
    .Q(output3),
    .SCD(output5),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK(clock1));
 sky130_fd_sc_hd__sdfbbn_1 ff4_clk2_falling (.D(port1),
    .Q(output14),
    .SCD(output16),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK_N(clock2));
 sky130_fd_sc_hd__sdfsbp_1 ff4_clk2_rising (.D(port1),
    .Q(output4),
    .SCD(output6),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK(clock2));
 sky130_fd_sc_hd__sdfbbn_1 ff5_clk1_falling (.D(port1),
    .Q(output15),
    .SCD(output17),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK_N(clock1));
 sky130_fd_sc_hd__sdfsbp_1 ff5_clk1_rising (.D(port1),
    .Q(output5),
    .SCD(output7),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK(clock1));
 sky130_fd_sc_hd__sdfbbn_1 ff6_clk2_falling (.D(port1),
    .Q(output16),
    .SCD(output18),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK_N(clock2));
 sky130_fd_sc_hd__sdfsbp_1 ff6_clk2_rising (.D(port1),
    .Q(output6),
    .SCD(isolated_wire),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK(clock2));
 sky130_fd_sc_hd__sdfbbn_1 ff7_clk1_falling (.D(port1),
    .Q(output17),
    .SCD(output19),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK_N(clock1));
 sky130_fd_sc_hd__sdfsbp_1 ff7_clk1_rising (.D(port1),
    .Q(output7),
    .SCD(output9),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK(clock1));
 sky130_fd_sc_hd__sdfbbn_1 ff8_clk2_falling (.D(port1),
    .Q(output18),
    .SCD(scan_in_3),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK_N(clock2));
 sky130_fd_sc_hd__sdfsbp_1 ff8_clk2_rising (.D(port1),
    .Q(output8),
    .SCD(scan_in_4),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK(clock2));
 sky130_fd_sc_hd__sdfbbn_1 ff9_clk1_falling (.D(port1),
    .Q(output19),
    .SCD(scan_in_1),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK_N(clock1));
 sky130_fd_sc_hd__sdfsbp_1 ff9_clk1_rising (.D(port1),
    .Q(output9),
    .SCD(scan_in_2),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK(clock1));
 sky130_fd_sc_hd__sdfbbn_1 ff10_clk2_falling (.D(port1),
    .Q(output20),
    .SCD(output12),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK_N(clock2));
 sky130_fd_sc_hd__sdfsbp_1 ff10_clk2_rising (.D(port1),
    .Q(output10),
    .SCD(output2),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK(clock2));
 sky130_fd_sc_hd__sdfbbn_1 ff1_clk1_falling (.D(port1),
    .Q(output11),
    .SCD(output13),
    .SCE(scan_enable_1),
    .SET_B(set_b),
    .CLK_N(clock1));
endmodule
