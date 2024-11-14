from openroad import Design, Tech
import helpers
import cts_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = helpers.make_design(tech)
design.readDef("no_clock.def")

design.evalTclString("set_wire_rc -clock -layer metal5")

try:
    cts_aux.clock_tree_synthesis(
        design,
        root_buf="CLKBUF_X3",
        buf_list="CLKBUF_X3",
        wire_unit=20,
        obstruction_aware=True,
    )
except Exception as inst:
    print(inst.args[0])

print("")  # so we match, tcl prints out extra line,
