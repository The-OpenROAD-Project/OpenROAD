module top (a,
    seed,
    out0,
    out1);
 input a;
 input seed;
 output out0;
 output out1;

 wire signal;

 Source source (.in(a),
    .out(signal));
 Sink sink (.out0(out0),
    .seed(seed),
    .out1(out1));
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
module Source (in,
    out);
 input in;
 output out;


 Driver driver (.in(in),
    .out(out));
endmodule
module Sink (seed,
    out0,
    out1);
 input seed;
 output out0;
 output out1;

 Load0 load0 (.i(seed),
    .out(out0));
 Load1 load1 (.i(seed),
    .out(out1));
endmodule
