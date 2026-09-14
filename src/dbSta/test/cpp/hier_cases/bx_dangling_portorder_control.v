// TOP: top
// TECH: nangate45
// TARGETS: port_order_control, no_dangling
// CLUE: control for port reordering seen in the dangling-top-input cases: fully
// connected top whose port list is deliberately NOT alphabetical
// (zb, ma, ay). If the writer still sorts, reordering is general.
module top (zb, ma, ay);
  input zb;
  input ma;
  output ay;
  AND2_X1 g1 (.A1(zb), .A2(ma), .ZN(ay));
endmodule
