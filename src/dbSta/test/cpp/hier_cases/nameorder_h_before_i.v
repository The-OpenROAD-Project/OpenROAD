// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, single_letter_pivot
// CLUE: single-letter pivot test: top wire h sorts just BEFORE input i. Predicted to break.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, output o);
  wire h;
  ft u0 (.a(i), .y(h));
  assign o = h;
endmodule
