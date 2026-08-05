module top (out0,
    out1,
    out2,
    foo);
 output out0;
 output out1;
 output out2;
 input [7:0] foo;


 SUB sub0 (.foo({foo[7],
    foo[6],
    foo[5],
    foo[4],
    foo[3],
    foo[2],
    foo[1],
    foo[0]}),
    .out0(out0),
    .out1(out1),
    .out2(out2));
endmodule
module CHILD (foo,
    Z);
 input [7:0] foo;
 output Z;


 BUF_X1 load1 (.A(foo[3]),
    .Z(Z));
endmodule
module SUB (foo,
    out0,
    out1,
    out2);
 input [7:0] foo;
 output out0;
 output out1;
 output out2;

 wire foo_3_;

 CHILD child0 (.foo({foo[7],
    foo[6],
    foo[5],
    foo[4],
    foo_3_,
    foo[2],
    foo[1],
    foo[0]}),
    .Z(out1));
 BUF_X1 load0 (.A(foo_3_),
    .Z(out0));
 BUF_X1 load2 (.A(foo[3]),
    .Z(out2));
 BUF_X4 split (.A(foo[3]),
    .Z(foo_3_));
endmodule
