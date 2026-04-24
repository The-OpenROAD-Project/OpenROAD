// Parallel pipelines with deliberately deep combinational paths, clocked
// impossibly fast by constraint.sdc. Used by the repair_timing WNS-stagnation
// regression: the flow must bail deterministically instead of grinding
// forever on a gap it cannot close. More pipelines => more endpoints =>
// repair_timing iterates long enough for the stagnation gate to trip.

`define LANES 8
`define DEPTH 8

module hopeless (
  input  wire                       clk,
  input  wire                       reset,
  input  wire [31:0]                a,
  input  wire [31:0]                b,
  output reg  [`LANES*32-1:0]       out
);

  reg [31:0] a_r, b_r;
  always @(posedge clk) begin
    a_r <= a;
    b_r <= b;
  end

  genvar g;
  generate
    for (g = 0; g < `LANES; g = g + 1) begin : lane
      wire [31:0] c0 = a_r ^ (b_r + g);
      wire [31:0] c1 = c0 + (32'hdeadbeef ^ g);
      wire [31:0] c2 = c1 ^ (32'h55555555 + g);
      wire [31:0] c3 = c2 + (32'h12345678 ^ g);
      wire [31:0] c4 = c3 ^ (32'haaaaaaaa + g);
      wire [31:0] c5 = c4 + (32'h0f0f0f0f ^ g);
      wire [31:0] c6 = c5 ^ (32'hf0f0f0f0 + g);
      wire [31:0] c7 = c6 + (32'hcafebabe ^ g);

      always @(posedge clk) begin
        if (reset) begin
          out[32*g +: 32] <= 32'b0;
        end else begin
          out[32*g +: 32] <= c7;
        end
      end
    end
  endgenerate

endmodule
