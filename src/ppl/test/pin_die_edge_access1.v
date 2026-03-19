// Minimal module to test pin placement at die edge.
// A few pins on different edges to trigger M5 placement on bottom.
module tiny_macro (
    input  wire       clock,
    input  wire [7:0] data_in,
    output wire [7:0] data_out,
    output wire       test_pin
);
    assign data_out = data_in;
    assign test_pin = 1'b0;
endmodule
