// rf8x4 as a SystemVerilog module: a parameter list, port widths written
// in its parameters, and an input the register file does not use.
module rf8x4 #(
    parameter int unsigned W = 4,
    parameter int unsigned A = 3
) (
    input  logic         clock,
    input  logic         test_en_i,
    input  logic [A-1:0] r0_addr,
    output logic [W-1:0] r0_data,
    input  logic [A-1:0] r1_addr,
    output logic [W-1:0] r1_data,
    input  logic [A-1:0] w0_addr,
    input  logic [W-1:0] w0_data,
    input  logic         w0_en,
    input  logic [A-1:0] w1_addr,
    input  logic [W-1:0] w1_data,
    input  logic         w1_en
);
endmodule
