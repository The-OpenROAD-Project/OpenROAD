`include "ram_2x8.v"
`include "sky_130hd_formal_pdk.v"

module RAM2x8_tb;

    reg clk;
    reg [7:0] D;
    reg [0:0] addr;
    reg [0:0] we;

    output wire [7:0] Q;

RAM2x8 dut (.clk(clk),
    .D(D),
    .Q(Q),
    .addr(addr),
    .we(we));

// initial begin
//     $dumpfile ("ram_8x8.vcd");
//     $dumpvars(0,RAM8x8_tb);


//     clk = 0;
//     we = 1;

//     #10 clk = 1;
//     D = 8'b11111111; addr = 3'b000;

//     #10 D = 8'b00000001; addr = 3'b001;
//     #10 clk = 1;

//     #10 clk = 0;
//     we = 0;

//     #10 clk = 1;

//     #10 addr = 3'b001;

//     $display("RAM_8x8 Test Complete");

// end


always begin
    #10 clk <= ~clk;
end

initial begin

    $dumpfile("ram_2x8.vcd");
    $dumpvars(0,RAM2x8_tb);
    D = 0;
    // addr = 0;

    //write mode
    clk = 0;
    we = 1;

    // for (integer i = 0l i < 8; i = i + 1) begin
    //     @(negedge clk)
    //     addr = i;
    //     D = i;
    // end

    //word 0
    @(negedge clk)
    D = 8'd0; addr = 1'b0;

    //word 1
    @(negedge clk)
    D = 8'd1; addr = 1'b1;

    // for (integer i = 0l i < 8; i = i + 1) begin
    //     @(negedge clk)
    //     addr = i;
    // end

    //read mode
    @(negedge clk)
    we = 0;

    //read word 0
    addr = 1'b0;

    @(negedge clk)
    addr = 1'b1;

    @(negedge clk)
    $display ("Test complete: RAM_2x8");
    $finish();


end

endmodule
