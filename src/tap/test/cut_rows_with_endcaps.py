from openroad import Design, Tech
import tap
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")
tech.readLef("Nangate45/fakeram45_64x7.lef")
design = helpers.make_design(tech)
design.readDef("boundary_macros_tapsplaced.def")

options = tap.Options()
design.getTapcell().cutRows(options)
