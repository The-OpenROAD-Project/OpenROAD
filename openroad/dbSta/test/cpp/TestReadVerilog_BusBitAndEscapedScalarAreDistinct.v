module top (foo,
    \foo[3] ,
    out_bus,
    out_scalar);
 input [7:0] foo;
 input \foo[3] ;
 output out_bus;
 output out_scalar;

 child u_child (.foo(foo),
    .\foo[3] (\foo[3] ),
    .out_bus(out_bus),
    .out_scalar(out_scalar));
endmodule

module child (foo,
    \foo[3] ,
    out_bus,
    out_scalar);
 input [7:0] foo;
 input \foo[3] ;
 output out_bus;
 output out_scalar;

 BUF_X1 bus_load (.A(foo[3]),
    .Z(out_bus));
 BUF_X1 scalar_load (.A(\foo[3] ),
    .Z(out_scalar));
endmodule
