// Reproduce the real bug: remove_buffers + insertBufferBeforeLoad.
// Hierarchical design where a submodule drives two output ports,
// with a buffer connecting port_a to port_b inside the submodule.
//
// After remove_buffers + insertBufferBeforeLoad (buggy):
//   VerilogWriter emits both submodule .Z_b(port_b) AND assign port_b = net1
//   → multi-driver on port_b
module top (port_a,
    port_b);
 output port_a;
 output port_b;


 MOD0 mod0 (.Z_a(port_a),
    .Z_b(port_b));
endmodule
module MOD0 (Z_a,
    Z_b);
 output Z_a;
 output Z_b;


 LOGIC0_X1 drvr (.Z(Z_a));
 BUF_X1 cross_buf (.A(Z_a),
    .Z(Z_b));
endmodule
