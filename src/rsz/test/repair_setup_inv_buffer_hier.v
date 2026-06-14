// Hierarchical version of repair_setup_inv_buffer:
// the BUF_X4 lives inside submodule m1 with both its A pin (driven by the
// top-level r1q wire crossing the module boundary) and its Z pin (driving the
// m1 output port that feeds top-level r2/D) carrying modnets after
// link_design -hier. This is the netlist shape that crashed InvBufferMove
// before the flat/hier net-handling fix.

module submod(in, out);
 input in;
 output out;
 BUF_X4 u1 (.A(in),
            .Z(out));
endmodule

module reg1(clk);
 input clk;
 wire r1q;
 wire u1z;
 DFF_X1 r1 (.D(1'b0),
            .CK(clk),
            .Q(r1q));
 submod m1 (.in(r1q),
            .out(u1z));
 DFF_X1 r2 (.D(u1z),
            .CK(clk));
endmodule
