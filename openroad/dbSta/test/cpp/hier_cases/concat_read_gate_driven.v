// TOP: top
// TECH: nangate45
// TARGETS: concat_read_of_gate_wire, control
// CLUE: control for T2: identical top-level concat read, but the internal wire is driven by a real cell inside the sub instead of a feedthrough assign.

module gsub (input a, output y);
  INV_X1 g0 (.A(a), .ZN(y));
endmodule

module top (input i, input k, output [1:0] o);
  wire m;
  gsub u0 (.a(i), .y(m));
  assign o = {k, m};
endmodule
