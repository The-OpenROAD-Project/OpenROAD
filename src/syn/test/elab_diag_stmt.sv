// A parallel block inside a procedure: the frontend only lowers sequential
// blocks, and raises an unimplemented-statement diagnostic naming the block.
module top (input wire clk, input wire x, output reg y);
  always @(posedge clk) begin
    fork
      y <= x;
    join
  end
endmodule
