module top (clk, in1, in2, out1);
  input clk, in1, in2;
  output out1;
  wire clk_buf, gclk, q0, en, q1, d2;
  wire n1, n2, n3, n4, n5, n6, n7;
  wire m1, m2, m3, m4, m5, m6, m7;

  BUFx2_ASAP7_75t_R clkbuf (.A(clk), .Y(clk_buf));
  DFFHQNx1_ASAP7_75t_R r0 (.CLK(clk_buf), .D(in1), .QN(q0));
  INVx1_ASAP7_75t_R u1 (.A(q0), .Y(n1));
  INVx1_ASAP7_75t_R u2 (.A(n1), .Y(n2));
  INVx1_ASAP7_75t_R u3 (.A(n2), .Y(n3));
  INVx1_ASAP7_75t_R u4 (.A(n3), .Y(n4));
  INVx1_ASAP7_75t_R u5 (.A(n4), .Y(n5));
  INVx1_ASAP7_75t_R u6 (.A(n5), .Y(n6));
  INVx1_ASAP7_75t_R u7 (.A(n6), .Y(n7));
  INVx1_ASAP7_75t_R u8 (.A(n7), .Y(en));
  ICGx1_ASAP7_75t_R icg (.CLK(clk_buf), .ENA(en), .SE(1'b0), .GCLK(gclk));
  DFFHQNx1_ASAP7_75t_R r1 (.CLK(gclk), .D(in2), .QN(q1));
  INVx1_ASAP7_75t_R v1 (.A(q1), .Y(m1));
  INVx1_ASAP7_75t_R v2 (.A(m1), .Y(m2));
  INVx1_ASAP7_75t_R v3 (.A(m2), .Y(m3));
  INVx1_ASAP7_75t_R v4 (.A(m3), .Y(m4));
  INVx1_ASAP7_75t_R v5 (.A(m4), .Y(m5));
  INVx1_ASAP7_75t_R v6 (.A(m5), .Y(m6));
  INVx1_ASAP7_75t_R v7 (.A(m6), .Y(m7));
  INVx1_ASAP7_75t_R v8 (.A(m7), .Y(d2));
  DFFHQNx1_ASAP7_75t_R r2 (.CLK(clk_buf), .D(d2), .QN(out1));
endmodule
