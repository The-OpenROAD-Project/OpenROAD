// Each bit of s depends only on lower bits of s, so the word-level
// feedback through the adder is not a loop once bitblasted.
module false_loop (
    input  [3:0] a,
    output [3:0] s
);
  assign s = a + {s[2:0], 1'b0};
endmodule

// Latch-based clock gate: the incomplete assignment infers a latch.
module latch_clock_gate (
    input  clk_i,
    input  en_i,
    output clk_o
);
  reg en_latch;
  always @* begin
    if (!clk_i) begin
      en_latch = en_i;
    end
  end
  assign clk_o = en_latch & clk_i;
endmodule

// Feedback through a RAM macro: its outputs have no liberty function, so
// it is opaque and breaks the loop.
module ram_loop (
    input        clk,
    input  [5:0] addr,
    input  [6:0] d,
    output [6:0] q
);
  fakeram45_64x7 ram (
      .clk(clk),
      .ce_in(1'b1),
      .we_in(1'b1),
      .addr_in(addr),
      .wd_in(q ^ d),
      .w_mask_in(7'h7f),
      .rd_out(q)
  );
endmodule

// DUAL has Y = A and Z = B, so feeding Y back into B is not a loop.
module dual_no_loop (
    input  a,
    output z
);
  wire y;
  DUAL u (
      .A(a),
      .B(y),
      .Y(y),
      .Z(z)
  );
endmodule

// HALF has Y = A and an output Z without a function; feeding Y back
// into A is a loop even though Z is opaque.
module half_loop (
    output z
);
  wire y;
  HALF u (
      .A(y),
      .Y(y),
      .Z(z)
  );
endmodule
