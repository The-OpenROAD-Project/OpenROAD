// TOP: top
// TECH: nangate45
// TARGETS: top_port_readback, sibling_chain
// CLUE: A TOP output net is also the source for another child's input port:
// CLUE: the net is simultaneously a primary output and an internal driver.

module top (a, y0, y1);
 input a;
 output y0, y1;
 sa u1 (.i(a), .o(y0));
 sb u2 (.i(y0), .o(y1));
endmodule

module sa (i, o);
 input i;
 output o;
 INV_X1 g (.A(i), .ZN(o));
endmodule

module sb (i, o);
 input i;
 output o;
 BUF_X1 g (.A(i), .Z(o));
endmodule
