from openroad import Design, Tech
import helpers
import rcx_aux

tech = Tech()
tech.readLef("sky130hs/sky130hs.tlef")
design = helpers.make_design(tech)

rcx_aux.bench_wires(design, len=100, all=True)

def_file = helpers.make_result_file("generate_pattern.def")

verilog_file = helpers.make_result_file("generate_pattern.v")

rcx_aux.bench_verilog(design, filename=verilog_file)

design.writeDef(def_file)

helpers.diff_files("generate_pattern.defok", def_file)
helpers.diff_files("generate_pattern.vok", verilog_file)
