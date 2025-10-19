module one_cell (clock,
    output1,
    port1,
    set_b,
    scan_enable_0,
    scan_in_0);
 input clock;
 output output1;
 input port1;
 input set_b;
 input scan_enable_0;
 input scan_in_0;


 sky130_fd_sc_hd__sdfsbp_1 ff1 (.D(port1),
    .Q(output1),
    .SCD(scan_in_0),
    .SCE(scan_enable_0),
    .SET_B(set_b),
    .CLK(clock));
endmodule
