from openroad import Design, Tech
import helpers
import rmp_aux
import rmp

tech = Tech()
design = helpers.make_design(tech)

blif = rmp_aux.create_blif(
    design, hicell="LOGIC1_X1", hiport="Z", locell="LOGIC0_X1", loport="Z"
)

tech.readLef("./Nangate45/Nangate45.lef")
tech.readLiberty("./Nangate45/Nangate45_typ.lib")
design.readDef("design_in_out.def")

block = design.getBlock()

blif.addReplaceableInstance(block.findInst("_i1_"))
blif.addReplaceableInstance(block.findInst("_i2_"))
blif.addReplaceableInstance(block.findInst("_i3_"))
blif.addReplaceableInstance(block.findInst("_i4_"))
blif.addReplaceableInstance(block.findInst("_i5_"))
blif.addReplaceableInstance(block.findInst("_i6_"))
blif.addReplaceableInstance(block.findInst("_i7_"))
blif.addReplaceableInstance(block.findInst("_i8_"))

blif.readBlif("./blif_reader.blif", block)

def_file = helpers.make_result_file("blif_reader.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "blif_reader.def.ok")
