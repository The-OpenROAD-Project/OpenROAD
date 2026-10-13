module top (\foo[3] ,
    out);
 input \foo[3] ;
 output out;

 child u_child (.\foo[3] (\foo[3] ),
    .out(out));
endmodule

module child (\foo[3] ,
    out);
 input \foo[3] ;
 output out;

 BUF_X1 load (.A(\foo[3] ),
    .Z(out));
endmodule
