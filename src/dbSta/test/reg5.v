// reg4 but with reversed bus order for in
module reg5 (out,
    clk,
    in);
 output out;
 input [2:0] clk;
 input [0:1] in;

 wire r1q;
 wire r2q;
 wire u1z;
 wire u2z;

 snl_ffqx1 r1 (.Q(r1q),
    .D(in[0]),
    .CP(clk[0]));
 snl_ffqx1 r2 (.Q(r2q),
    .D(in[1]),
    .CP(clk[1]));
 snl_ffqx1 r3 (.Q(out),
    .D(u2z),
    .CP(clk[2]));
 snl_bufx1 u1 (.Z(u1z),
    .A(r2q));
 snl_and02x1 u2 (.Z(u2z),
    .A(r1q),
    .B(u1z));
endmodule
