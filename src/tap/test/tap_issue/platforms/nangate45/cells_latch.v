module $_DLATCH_P_(input E, input D, output Q);
    DLH_X1 _TECHMAP_REPLACE_ (
        .D(D),
        .G(E),
        .Q(Q)
        );
endmodule

module $_DLATCH_N_(input E, input D, output Q);
    DLL_X1 _TECHMAP_REPLACE_ (
        .D(D),
        .GN(E),
        .Q(Q)
        );
endmodule