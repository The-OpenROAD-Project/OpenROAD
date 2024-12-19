import odb
import helper
import odbUnitTest
import unittest


def placeInst(inst, x, y):
    inst.setLocation(x, y)
    inst.setPlacementStatus("PLACED")


def placeBPin(bpin, layer, x1, y1, x2, y2):
    odb.dbBox_create(bpin, layer, x1, y1, x2, y2)
    bpin.setPlacementStatus("PLACED")


class TestBlock(odbUnitTest.TestCase):
    def setUp(self):
        self.db, self.lib = helper.createSimpleDB()
        self.parentBlock = odb.dbBlock_create(self.db.getChip(), "Parent")
        self.block = helper.create2LevelBlock(self.db, self.lib, self.parentBlock)


if __name__ == "__main__":
    unittest.main()
