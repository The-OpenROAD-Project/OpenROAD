module top (a,
    out0,
    out1,
    seed);
 input a;
 output out0;
 output out1;
 input seed;

 wire signal;

 Sink sink (.net(signal),
    .seed(seed),
    .out0(out0),
    .out1(out1));
 Source source (.in(a),
    .out(signal));
endmodule
module Driver (in,
    out);
 input in;
 output out;


 INV_X2 u_drv (.A(in),
    .ZN(out));
endmodule
module Load0 (i,
    out);
 input i;
 output out;


 INV_X1 u_inv (.A(i),
    .ZN(out));
endmodule
module Load1 (i,
    out);
 input i;
 output out;


 INV_X1 u_inv (.A(i),
    .ZN(out));
endmodule
module Sink (net,
    seed,
    out0,
    out1);
 input net;
 input seed;
 output out0;
 output out1;


 Load0 load0 (.i(net),
    .out(out0));
 Load1 load1 (.i(net),
    .out(out1));
endmodule
module Source (in,
    out);
 input in;
 output out;


 Driver driver (.in(in),
    .out(out));
endmodule
