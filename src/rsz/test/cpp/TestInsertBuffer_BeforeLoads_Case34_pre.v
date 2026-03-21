// Bug reproduction: makeNewNetName does not check ModNet/ModBTerm names
// when creating a new flat net. In hierarchical designs, a module port
// named "net" creates a ModBTerm "net", but the corresponding flat net
// has a different hierarchical name. insertBufferBeforeLoads with
// IF_NEEDED uniquify creates flat net "sub0/net" that collides with
// the existing ModBTerm "net", producing duplicate "wire net;" in the
// emitted Verilog.
//
// This reproduces the sky130hd/microwatt CTS LEC failure:
//   [critical] wire collision for net net
module top (in,
    out_net,
    out1,
    out2);
 input in;
 output out_net;
 output out1;
 output out2;

 wire n_internal;

 SUB sub0 (.A(in),
    .net(n_internal),
    .Z1(out1),
    .Z2(out2));
 BUF_X1 extra (.A(n_internal),
    .Z(out_net));
endmodule
module SUB (A,
    net,
    Z1,
    Z2);
 input A;
 output net;
 output Z1;
 output Z2;


 BUF_X1 drv (.A(A),
    .Z(net));
 BUF_X1 load1 (.A(net),
    .Z(Z1));
 BUF_X1 load2 (.A(net),
    .Z(Z2));
endmodule
