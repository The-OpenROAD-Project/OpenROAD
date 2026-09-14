// TOP: top
// TECH: nangate45
// TARGETS: attribute, dont_touch, stoi_exception
// CLUE: makeChildInsts does std::stoi(network_->getAttribute(child,
// "dont_touch")) with no try/catch (dbReadVerilog.cc:580). The Verilog grammar
// accepts a STRING attribute value (VerilogParse.yy attr_spec_value), so
// (* dont_touch = "true" *) -- the spelling most synthesis flows emit -- hands
// stoi a non-numeric string.
module top (a, y);
   input a;
   output y;
   (* dont_touch = "true" *)
   INV_X1 g (.A(a), .ZN(y));
endmodule
