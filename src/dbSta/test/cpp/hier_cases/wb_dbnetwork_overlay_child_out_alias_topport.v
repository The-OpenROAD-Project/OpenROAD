// TOP: top
// TECH: nangate45
// TARGETS: hier, output_port_alias, top_output_receiver, pivot
// CLUE: pivot for ..._child_out_alias_internal: identical child, but the aliased
// output feeds a top OUTPUT PORT directly. The campaign's known finding 1 shape
// (extra top-level assign on an already-driven output) is expected here; running
// it next to the internal-receiver variant tells us whether the top-port case is
// special-cased by writeAssigns or whether both shapes share one root cause.
module top (a, y);
   input a;
   output y;
   m u1 (.i(a), .o(y));
endmodule

module m (i, o);
   input i;
   output o;
   wire w;
   INV_X1 g1 (.A(i), .ZN(w));
   assign o = w;
endmodule
