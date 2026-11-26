// A module read after the design was loaded: a replacement for
// replace_hier_mod1.v's buffer_chain, two buffers instead of its chain.
module buffer_pair (input I,
    output O);
 wire n1;

 BUF_X1 p1 (.A(I),
    .Z(n1));
 BUF_X1 p2 (.A(n1),
    .Z(O));
endmodule
