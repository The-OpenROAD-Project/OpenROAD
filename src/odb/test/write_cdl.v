module top ( in_reg, out_reg );
    output [1:0] in_reg;
    output [1:0] out_reg;
    MACRO_CELL i_macro (
        .IN_REG(in_reg),
        .OUT_REG(out_reg)
  );
endmodule
