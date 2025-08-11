// A standard cell definition for the test.
// Defined as a blackbox to prevent yosys from optimizing it away.
module and2 (input A, input B, output Z);
endmodule

// Module to test ORD-2043: defined but never instantiated
module unused_module (input A, output Z);
   assign Z = A;
endmodule

// Module to test ORD-2038: module with no instances
module empty_module ();
endmodule

module top (
    input clk,
    input top_in,
    output top_out,
    output out_unconnected // For ORD-2040
);

    // For ORD-2038: An instance of a module that has no instances inside.
    empty_module i_empty();

    // For ORD-2041: non-output ITerm not connected.
    wire w1, w2;
    and2 u1 (.A(w1), .B(), .Z(w2));

    // For ORD-2039: Net with < 2 connections.
    wire single_conn_wire;
    and2 u2 (.A(single_conn_wire), .B(top_in), .Z());

    wire dangling_wire; // another case for ORD-2039

    // Normal connection to make the design valid for linking.
    assign top_out = w2;
    assign w1 = top_in;

endmodule