// TOP: top
// TECH: nangate45
// TARGETS: dollar_name, port, depth_1
// CLUE: $ in top-level port names (in$1, sel$, out$); boundary name matching.
module top (in$1, sel$, out$);
  input in$1, sel$;
  output out$;
  wire n1;
  AND2_X1 u1 (.A1(in$1), .A2(sel$), .ZN(n1));
  INV_X1 u2 (.A(n1), .ZN(out$));
endmodule
