// Quick and dirty reimplementation of DFFHQNx2_ASAP7_75t_R because
// Verialtor doesn't support and has no plans to support 1995 UDP
// tables.
//
// https://github.com/verilator/verilator/issues/5243

module DFFHQNx1_ASAP7_75t_R (QN, D, CLK);
    output reg QN;
    input D, CLK;

    always @(posedge CLK) begin
        QN <= ~D;
    end
endmodule

module DFFHQNx2_ASAP7_75t_R (QN, D, CLK);
    output reg QN;
    input D, CLK;

    always @(posedge CLK) begin
        QN <= ~D;
    end
endmodule

module DFFHQNx3_ASAP7_75t_R (QN, D, CLK);
    output reg QN;
    input D, CLK;

    always @(posedge CLK) begin
        QN <= ~D;
    end
endmodule
