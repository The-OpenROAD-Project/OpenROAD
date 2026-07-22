// MockArray - Idiomatic SystemVerilog rewrite of MockArray.scala
//
// Parameterized 2D array of Element modules, each containing 4 Multiplier
// instances with registered crossbar routing. Used as a test design for
// OpenROAD-flow-scripts.

// Blackbox stub for the gate-level multiplier (Amaranth-generated).
// The actual implementation lives in multiplier.v and is joined
// after hierarchical synthesis via genrule.
module multiplier(
  input  logic [31:0] b,
  input  logic        clk,
  input  logic        rst,
  output logic [31:0] o,
  input  logic [31:0] a
);
endmodule

// Multiplier wrapper - reduces blackbox output to bits [3:0].
// Kept as a separate module for hierarchical power testing.
module Multiplier(
  input  logic [31:0] a,
  input  logic [31:0] b,
  output logic [31:0] o,
  input  logic        rst,
  input  logic        clk
);
  logic [31:0] mult_o;
  multiplier mult_inst(
    .a(a),
    .b(b),
    .o(mult_o),
    .rst(rst),
    .clk(clk)
  );
  assign o = {28'b0, mult_o[3:0]};
endmodule

// ElementInner - parameterized single cell in the MockArray grid.
//
// Registered crossbar routing with multipliers:
//   out_left  = reg(mult(reg(in_down),  reg(in_left)))
//   out_up    = reg(mult(reg(in_right), reg(in_down)))
//   out_right = reg(mult(reg(in_up),    reg(in_right)))
//   out_down  = reg(mult(reg(in_left),  reg(in_up)))
//
// LSB chain: combinational shift with a register every COLS/2 steps.
module ElementInner #(
  parameter int DATA_WIDTH = 64,
  parameter int COLS       = 8
)(
  input  logic                  clock,
  input  logic [DATA_WIDTH-1:0] io_ins_left,
  input  logic [DATA_WIDTH-1:0] io_ins_up,
  input  logic [DATA_WIDTH-1:0] io_ins_right,
  input  logic [DATA_WIDTH-1:0] io_ins_down,
  output logic [DATA_WIDTH-1:0] io_outs_left,
  output logic [DATA_WIDTH-1:0] io_outs_up,
  output logic [DATA_WIDTH-1:0] io_outs_right,
  output logic [DATA_WIDTH-1:0] io_outs_down,
  input  logic [COLS-1:0]       io_lsbIns,
  output logic [COLS-1:0]       io_lsbOuts
);
  localparam int MAX_FLIGHT = COLS / 2;

  // Stage 1: register all directional inputs
  logic [DATA_WIDTH-1:0] left_r, up_r, right_r, down_r;
  always_ff @(posedge clock) begin : reg_inputs
    left_r  <= io_ins_left;
    up_r    <= io_ins_up;
    right_r <= io_ins_right;
    down_r  <= io_ins_down;
  end

  // Stage 2: multiplier crossbar
  logic [31:0] mult_left_o, mult_up_o, mult_right_o, mult_down_o;

  Multiplier mult_left(
    .a(down_r[31:0]),  .b(left_r[31:0]),
    .o(mult_left_o),   .rst(1'b0), .clk(clock)
  );
  Multiplier mult_up(
    .a(right_r[31:0]), .b(down_r[31:0]),
    .o(mult_up_o),     .rst(1'b0), .clk(clock)
  );
  Multiplier mult_right(
    .a(up_r[31:0]),    .b(right_r[31:0]),
    .o(mult_right_o),  .rst(1'b0), .clk(clock)
  );
  Multiplier mult_down(
    .a(left_r[31:0]),  .b(up_r[31:0]),
    .o(mult_down_o),   .rst(1'b0), .clk(clock)
  );

  // Stage 3: register multiplier outputs (zero-extended to DATA_WIDTH)
  always_ff @(posedge clock) begin : reg_outputs
    io_outs_left  <= mult_left_o;
    io_outs_up    <= mult_up_o;
    io_outs_right <= mult_right_o;
    io_outs_down  <= mult_down_o;
  end

  // LSB chain: shift lsbIns left by one position.
  // Insert a register break every MAX_FLIGHT positions to limit
  // combinational path length to COLS/2 elements.
  logic [COLS-1:0] lsb_reg;
  always_ff @(posedge clock) begin : reg_lsb
    for (int j = 0; j < COLS - 1; j++) begin
      if ((COLS - 1 - j) % MAX_FLIGHT == 0)
        lsb_reg[j] <= io_lsbIns[j + 1];
    end
  end

  always_comb begin : comb_lsb
    for (int j = 0; j < COLS - 1; j++) begin
      if ((COLS - 1 - j) % MAX_FLIGHT == 0)
        io_lsbOuts[j] = lsb_reg[j];
      else
        io_lsbOuts[j] = io_lsbIns[j + 1];
    end
    io_lsbOuts[COLS - 1] = io_outs_left[0];
  end
endmodule

// Element - concrete wrapper for synthesis as a standalone macro.
// Uses `define to set COLS per config since OpenROAD cannot read
// parameterized Verilog instantiations.
//
// ELEMENT_COLS must be defined at synthesis time via -D flag, e.g.:
//   SYNTH_SLANG_ARGS: "-DELEMENT_COLS=4"
`ifndef ELEMENT_COLS
`define ELEMENT_COLS 8
`endif
module Element(
  input  logic        clock,
  input  logic [63:0] io_ins_left,
  input  logic [63:0] io_ins_up,
  input  logic [63:0] io_ins_right,
  input  logic [63:0] io_ins_down,
  output logic [63:0] io_outs_left,
  output logic [63:0] io_outs_up,
  output logic [63:0] io_outs_right,
  output logic [63:0] io_outs_down,
  input  logic [`ELEMENT_COLS-1:0] io_lsbIns,
  output logic [`ELEMENT_COLS-1:0] io_lsbOuts
);
  ElementInner #(.DATA_WIDTH(64), .COLS(`ELEMENT_COLS)) inner(.*);
endmodule

// MockArray - parameterized 2D array of Elements
module MockArray #(
  parameter int WIDTH      = 8,
  parameter int HEIGHT     = 8,
  parameter int DATA_WIDTH = 64
)(
  input  logic                  clock,
  input  logic                  reset,
  input  logic [DATA_WIDTH-1:0] io_ins_left   [HEIGHT],
  input  logic [DATA_WIDTH-1:0] io_ins_right  [HEIGHT],
  input  logic [DATA_WIDTH-1:0] io_ins_up     [WIDTH],
  input  logic [DATA_WIDTH-1:0] io_ins_down   [WIDTH],
  output logic [DATA_WIDTH-1:0] io_outs_left  [HEIGHT],
  output logic [DATA_WIDTH-1:0] io_outs_right [HEIGHT],
  output logic [DATA_WIDTH-1:0] io_outs_up    [WIDTH],
  output logic [DATA_WIDTH-1:0] io_outs_down  [WIDTH],
  output logic [WIDTH*HEIGHT-1:0] io_lsbs
);

  // Element grid interconnect
  logic [DATA_WIDTH-1:0] e_ins_left   [HEIGHT][WIDTH];
  logic [DATA_WIDTH-1:0] e_ins_up     [HEIGHT][WIDTH];
  logic [DATA_WIDTH-1:0] e_ins_right  [HEIGHT][WIDTH];
  logic [DATA_WIDTH-1:0] e_ins_down   [HEIGHT][WIDTH];
  logic [DATA_WIDTH-1:0] e_outs_left  [HEIGHT][WIDTH];
  logic [DATA_WIDTH-1:0] e_outs_up    [HEIGHT][WIDTH];
  logic [DATA_WIDTH-1:0] e_outs_right [HEIGHT][WIDTH];
  logic [DATA_WIDTH-1:0] e_outs_down  [HEIGHT][WIDTH];
  logic [WIDTH-1:0]      e_lsbIns     [HEIGHT][WIDTH];
  logic [WIDTH-1:0]      e_lsbOuts    [HEIGHT][WIDTH];

  // Instantiate Element grid. Generate labels must be unique within
  // their enclosing scope; the instance name ces keeps the leaf
  // identifier matching the power/parasitics scripts.
  for (genvar r = 0; r < HEIGHT; r++) begin : ces_row
    for (genvar c = 0; c < WIDTH; c++) begin : ces_col
      // Element parameters (DATA_WIDTH, COLS) are set via
      // VERILOG_TOP_PARAMS at synthesis time - do not override here
      // as OpenROAD cannot read parameterized instantiations.
      Element ces(
        .clock       (clock),
        .io_ins_left (e_ins_left  [r][c]),
        .io_ins_up   (e_ins_up    [r][c]),
        .io_ins_right(e_ins_right [r][c]),
        .io_ins_down (e_ins_down  [r][c]),
        .io_outs_left (e_outs_left [r][c]),
        .io_outs_up   (e_outs_up   [r][c]),
        .io_outs_right(e_outs_right[r][c]),
        .io_outs_down (e_outs_down [r][c]),
        .io_lsbIns   (e_lsbIns    [r][c]),
        .io_lsbOuts  (e_lsbOuts   [r][c])
      );
    end
  end

  // --- Edge connectivity: top-level inputs to border elements ---
  for (genvar r = 0; r < HEIGHT; r++) begin : gen_edge_lr
    assign e_ins_right[r][0]        = io_ins_right[r];
    assign e_ins_left [r][WIDTH-1]  = io_ins_left[r];
  end
  for (genvar c = 0; c < WIDTH; c++) begin : gen_edge_ud
    assign e_ins_down[HEIGHT-1][c]  = io_ins_down[c];
    assign e_ins_up  [0][c]         = io_ins_up[c];
  end

  // --- Edge connectivity: border elements to top-level outputs ---
  for (genvar r = 0; r < HEIGHT; r++) begin : gen_out_lr
    assign io_outs_left [r] = e_outs_left [r][0];
    assign io_outs_right[r] = e_outs_right[r][WIDTH-1];
  end
  for (genvar c = 0; c < WIDTH; c++) begin : gen_out_ud
    assign io_outs_up  [c] = e_outs_up  [HEIGHT-1][c];
    assign io_outs_down[c] = e_outs_down[0][c];
  end

  // --- Neighbor connectivity: left/right ---
  for (genvar r = 0; r < HEIGHT; r++) begin : gen_nb_lr
    for (genvar c = 0; c < WIDTH - 1; c++) begin : gen_nb_col
      assign e_ins_left [r][c]   = e_outs_left [r][c+1];
      assign e_ins_right[r][c+1] = e_outs_right[r][c];
    end
  end

  // --- Neighbor connectivity: up/down ---
  for (genvar r = 0; r < HEIGHT - 1; r++) begin : gen_nb_ud
    for (genvar c = 0; c < WIDTH; c++) begin : gen_nb_col
      assign e_ins_down[r][c]   = e_outs_down[r+1][c];
      assign e_ins_up  [r+1][c] = e_outs_up  [r][c];
    end
  end

  // --- LSB chain: left-to-right within each row ---
  for (genvar r = 0; r < HEIGHT; r++) begin : gen_lsb_chain
    assign e_lsbIns[r][0] = '0;
    for (genvar c = 1; c < WIDTH; c++) begin : gen_lsb_col
      assign e_lsbIns[r][c] = e_lsbOuts[r][c-1];
    end
  end

  // --- Output LSBs: registered last-column lsbOuts ---
  logic [WIDTH*HEIGHT-1:0] lsbs_pre;
  for (genvar r = 0; r < HEIGHT; r++) begin : gen_lsbs_concat
    assign lsbs_pre[r*WIDTH +: WIDTH] = e_lsbOuts[r][WIDTH-1];
  end
  always_ff @(posedge clock) begin : reg_lsbs
    io_lsbs <= lsbs_pre;
  end

endmodule

