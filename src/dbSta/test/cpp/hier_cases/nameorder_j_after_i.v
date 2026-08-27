// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, single_letter_pivot, control
// CLUE: single-letter pivot control: top wire j sorts just AFTER input i. Predicted to pass.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input i, output o);
  wire j;
  ft u0 (.a(i), .y(j));
  assign o = j;
endmodule
