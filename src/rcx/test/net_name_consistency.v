module top();
  wire mid;
  wire w1;
  wire w0;
   
  INV_X1 leaf (.A(w0), .ZN(mid));
  block b(.i(mid), .o({w1, w0}));
  INV_X1 load0 (.A(w0));
  INV_X1 load1 (.A(w1));
endmodule

module block(input i, output [1:0] o);
  wire net;

  LOGIC0_X1 tie (.Z(net));
  INV_X1 leaf (.A(net), .ZN(o[0]));
  BUF_X1 buf0 (.A(net), .Z(o[1]));
endmodule
