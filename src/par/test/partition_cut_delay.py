# Verify that omitted and explicit cut delays match the Tcl command behavior.
from openroad import Tech
import helpers
import par_aux

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readVerilog("gcd.v")
design.link("gcd")
design.evalTclString("create_clock [get_ports clk] -name core_clock -period 0.25")
design.evalTclString(
    "set_input_delay 0 -clock core_clock [delete_from_list [all_inputs] [get_ports clk]]"
)
design.evalTclString("set_output_delay 0 -clock core_clock [all_outputs]")

solutions = []
for delay in (1e-9, None, 0):
    part_file = helpers.make_result_file(f"partition_cut_delay_{delay}.part")
    par_aux.tritonPartDesign(design, solution_file=part_file, extra_delay=delay)
    with open(part_file) as stream:
        solutions.append(stream.read())

if solutions[1] != solutions[2]:
    raise RuntimeError(
        "Omitting extra_delay changed the partition compared with zero delay"
    )
if solutions[0] == solutions[2]:
    raise RuntimeError(
        "Explicit cut delay did not change the partition without guardbanding"
    )
print("pass")
