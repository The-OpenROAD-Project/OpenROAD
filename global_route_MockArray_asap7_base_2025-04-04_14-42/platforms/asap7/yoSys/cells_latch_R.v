module $_DLATCH_P_(input E, input D, output Q);
    DHLx1_ASAP7_75t_R _TECHMAP_REPLACE_ (
        .D(D),
        .CLK(E),
        .Q(Q)
        );
endmodule

module $_DLATCH_N_(input E, input D, output Q);
    DLLx1_ASAP7_75t_R _TECHMAP_REPLACE_ (
        .D(D),
        .CLK(E),
        .Q(Q)
        );
endmodule
