module complex_propagation(input x, input y, output o);
    wire w1;
    wire w2;
    
    LOGIC1_X1 _01_ (w1);
    LOGIC0_X1 _02_ (w2);

    simple_module1 _03_ (x, y, w1, w2, o);
endmodule

module simple_module1(input x, input y, input z1, input z2, output o);
    wire o1;
    wire o2;

    simple_module2 _01_ (x, z1, o1);
    simple_module3 _02_ (y, z2, o1, o2);
    simple_module4 _03_ (o2, z1, o);
endmodule

module simple_module2(input x, input y, output o);
    AND2_X1 _02_(x, y, o);
endmodule

module simple_module3(input x, input y, input z, output o);
    AND3_X1 _02_(x, y, z, o);
endmodule

module simple_module4(input x, input y, output o);
    wire w1;
    simple_module5 _01_(x, y, w1);
    OR2_X1 _02_(x, w1, o);
endmodule

module simple_module5(input x, input y, output o);
    OR2_X1 _02_(x, y, o);
endmodule