module top (foo,
    out0,
    out1,
    out2);
 input [7:0] foo;
 output out0;
 output out1;
 output out2;


 SUB sub0 (.foo(foo),
    .out0(out0),
    .out1(out1),
    .out2(out2));
endmodule
module SUB (foo,
    out0,
    out1,
    out2);
 input [7:0] foo;
 output out0;
 output out1;
 output out2;


 BUF_X1 load0 (.A(foo[3]),
    .Z(out0));
 CHILD child0 (.foo(foo),
    .Z(out1));
 BUF_X1 load2 (.A(foo[3]),
    .Z(out2));
endmodule
module CHILD (foo,
    Z);
 input [7:0] foo;
 output Z;


 BUF_X1 load1 (.A(foo[3]),
    .Z(Z));
endmodule
