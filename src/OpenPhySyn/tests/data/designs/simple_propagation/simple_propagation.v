module simple_propagation(input x, input y, output o);
    wire w1;
    LOGIC1_X1 _01_ (w1);
    simple_module mod(x, y, w1, o);
endmodule

module simple_module(input x, input y, input z, output o);
    wire o1;
    AND2_X1 _01_ (x, y, o1);
    OR2_X1 _02_(o1, z, o);
endmodule