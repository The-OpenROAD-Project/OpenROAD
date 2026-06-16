module feedthrough (tap_in,
    tap_out);
 input tap_in;
 output tap_out;

 assign tap_out = tap_in;
endmodule

module top (clk,
    in1,
    out1,
    out2);
 input clk;
 input in1;
 output out1;
 output out2;

 wire tap_out;
 wire src_net;
 wire c0;

 DFF_X1 drv_ff (.D(in1),
    .CK(clk),
    .Q(src_net),
    .QN());
 BUF_X1 probe (.A(src_net),
    .Z(out2));
 OR2_X1 target (.A1(tap_out),
    .A2(c0),
    .ZN(out1));
 LOGIC0_X1 tie0 (.Z(c0));
 feedthrough u_ft (.tap_in(src_net),
    .tap_out(tap_out));
endmodule
