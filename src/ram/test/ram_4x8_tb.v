`include "ram_4x8.v"
`include "sky_130hd_formal_pdk.v"

module RAM4x8_tb;

    reg clk;
    reg [7:0] D;
    reg [1:0] addr;
    reg [0:0] we;

    output wire [7:0] Q;

RAM4x8 dut (.clk(clk),
    .D(D),
    .Q(Q),
    .addr(addr),
    .we(we));


always begin
    #10 clk <= ~clk;
end

initial begin

    $dumpfile("ram_4x8.vcd");
    $dumpvars(0,RAM4x8_tb);

    //write mode
    clk = 0;
    we = 1;
    D = 0;
    addr = 0;


    // for (integer i = 0l i < 8; i = i + 1) begin
    //     @(negedge clk)
    //     addr = i;
    //     D = i;
    // end

    //word 0
    @(negedge clk)
    D = 8'd0; addr = 2'd0;

    //word 1
    @(negedge clk)
    D = 8'd1; addr = 2'd1;

    //word 2
    @(negedge clk)
    D = 8'd2; addr = 2'd2;

    //word 3
    @(negedge clk)
    D = 8'd3; addr = 2'd3;


    // for (integer i = 0l i < 8; i = i + 1) begin
    //     @(negedge clk)
    //     addr = i;
    // end

    //read mode
    @(negedge clk)
    we = 0;

    addr = 2'd0;//set address to 0

    //read word 1
    @(negedge clk)
    addr = 2'd1;

    //read word 2
    @(negedge clk)
    addr = 2'd2;

    //read word 3
    @(negedge clk)
    addr = 2'd3;

    @(negedge clk)
    $display ("Test complete: RAM_4x8");
    $finish();


end

endmodule
