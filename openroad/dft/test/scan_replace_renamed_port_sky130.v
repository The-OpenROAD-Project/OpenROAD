module top (clock, port1, output1, output2);
  input clock, port1;
  output output1, output2;

  sky130_fd_sc_hd__dfxbp_1 ff1 (.CLK(clock), .D(port1), .Q(output1),
    .Q_N(output2));
endmodule
