module repair_hold_multi_output_load (
  input clk,
  input d_in
);
  wire launch_qn;
  wire con_net;
  wire sn_net;
  wire capture_con_qn;
  wire capture_sn_qn;

  DFFHQNx1_ASAP7_75t_R u_launch (
    .CLK(clk),
    .D(d_in),
    .QN(launch_qn)
  );

  FAx1_ASAP7_75t_R u_fa (
    .A(launch_qn),
    .B(1'b1),
    .CI(1'b0),
    .CON(con_net),
    .SN(sn_net)
  );

  DFFHQNx1_ASAP7_75t_R u_capture_con (
    .CLK(clk),
    .D(con_net),
    .QN(capture_con_qn)
  );

  DFFHQNx1_ASAP7_75t_R u_capture_sn (
    .CLK(clk),
    .D(sn_net),
    .QN(capture_sn_qn)
  );
endmodule
