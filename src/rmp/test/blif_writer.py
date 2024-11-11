from openroad import Design, Tech
import helpers
import rmp_aux
import rmp

tech = Tech()
tech.readLef("./Nangate45/Nangate45.lef")
tech.readLiberty("./Nangate45/Nangate45_typ.lib")
design = helpers.make_design(tech)
design.readDef("./design.def")

blif = rmp_aux.create_blif(design)

blif.addReplaceableInstance(design.getBlock().findInst("_i1_"))
blif.addReplaceableInstance(design.getBlock().findInst("_i2_"))
blif.addReplaceableInstance(design.getBlock().findInst("_i3_"))

blif_file = helpers.make_result_file("blif_writer.blif")

blif.writeBlif(blif_file)

helpers.diff_files(blif_file, "blif_writer.blif.ok")
