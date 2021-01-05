import opendbpy as odb
import os 

current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")

db = odb.dbDatabase.create()
odb.read_lef(db, os.path.join(data_dir, "gscl45nm.lef"))
odb.read_def(db, os.path.join(data_dir, "design.def"))
chip = db.getChip()
block = chip.getBlock()
bterms = block.getBTerms()


assert(len(bterms) == 12)
pins = bterms[0].getBPins()
assert(len(pins) == 2)
boxes = pins[0].getBoxes()
assert(len(boxes) == 2)
box_0 = boxes[0]
box_1 = boxes[1]


assert(box_0.xMin() == 19950)
assert(box_0.yMin() == 10000)
assert(box_0.xMax() == 20050)
assert(box_0.yMax() == 10100)


assert(box_1.xMin()  == 19980)
assert(box_1.yMin() == 10000)
assert(box_1.xMax()  == 20020)
assert(box_1.yMax() == 10150)





        

result = odb.write_def(block, os.path.join(opendb_dir, "build/def.out"))


exit()