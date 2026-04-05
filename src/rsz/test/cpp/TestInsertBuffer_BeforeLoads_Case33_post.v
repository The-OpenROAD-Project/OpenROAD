module top (port_a,
    port_b);
 output port_a;
 output port_b;

 wire net1;

 MOD0 mod0 (.Z_a(port_a),
    .Z_b(net1));
 BUF_X4 output1 (.A(net1),
    .Z(port_b));
endmodule
module MOD0 (Z_a,
    Z_b);
 output Z_a;
 output Z_b;


 LOGIC0_X1 drvr (.Z(Z_a));
 assign Z_b = Z_a;
endmodule
