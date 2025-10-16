module top (clk1_top,
    clk2_top,
    enable_top,
    reset_top,
    inputs_32_top,
    inputs_64_top,
    outputs_32_top,
    outputs_64_top,
    scan_enable_1_top,
    scan_in_1_top,
    scan_in_2_top);
 input clk1_top;
 input clk2_top;
 input enable_top;
 input reset_top;
 input [31:0] inputs_32_top;
 input [63:0] inputs_64_top;
 output [31:0] outputs_32_top;
 output [63:0] outputs_64_top;
 input scan_enable_1_top;
 input scan_in_1_top;
 input scan_in_2_top;

 bank1 my_bank1(.inputs_32(inputs_32_top), .outputs_32(outputs_32_top), .clk1(clk1_top), .enable(enable_top), .reset(reset_top), .scan_enable_1(scan_enable_1_top), .scan_in_1(scan_in_1_top));
 bank2 my_bank2(.inputs_64(inputs_64_top), .outputs_64(outputs_64_top), .clk2(clk2_top), .enable(enable_top), .reset(reset_top), .scan_enable_1(scan_enable_1_top), .scan_in_2(scan_in_2_top));

endmodule
