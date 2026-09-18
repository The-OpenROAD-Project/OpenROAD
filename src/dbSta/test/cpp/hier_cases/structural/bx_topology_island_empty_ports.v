// TOP: top
// TECH: nangate45
// TARGETS: empty_module_with_ports, undriven_output_port
// CLUE: module with ports but an EMPTY body (input dangles inside, output
// undriven inside) instantiated with real connections into a dangling net;
// observe whether the empty definition and its instance survive.

module ep (input i, output o);
endmodule

module top (input a, output z);
  wire nc;
  BUF_X1 gb (.A(a), .Z(z));
  ep u_ep (.i(a), .o(nc));
endmodule
