import opendb as odb
import helper
import odbUnitTest

def placeInst(inst, x, y):
    inst.setLocation(x, y)
    inst.setPlacementStatus('PLACED')
    
def placeBPin(bpin, layer, x1, y1, x2, y2):
    odb.dbBox_create(bpin, layer, x1, y1, x2, y2)
    bpin.setPlacementStatus('PLACED')
    
class TestBlock(odbUnitTest.TestCase):
    def setUp(self):
        self.db, self.lib = helper.createSimpleDB()
        self.parentBlock = odb.dbBlock_create(self.db.getChip(), 'Parent')
        self.block = helper.create2LevelBlock(self.db, self.lib, self.parentBlock)
        self.block.setCornerCount(4)
        self.extcornerblock = self.block.createExtCornerBlock(1)
        odb.dbTechNonDefaultRule_create(self.block, 'non_default_1')
        self.parentRegion = odb.dbRegion_create(self.block, 'parentRegion')
        self.childRegion = odb.dbRegion_create(self.parentRegion, 'childRegion')
        

    def tearDown(self):
        self.db.destroy(self.db)
    def test_find(self):
        #bterm
        self.assertEqual(self.block.findBTerm('IN1').getName(), 'IN1')
        self.assertIsNone(self.block.findBTerm('in1'))
        #child
        self.assertEqual(self.parentBlock.findChild('2LevelBlock').getName(), '2LevelBlock')
        self.assertIsNone(self.parentBlock.findChild('1LevelBlock'))
        #inst
        self.assertEqual(self.block.findInst('i3').getName(), 'i3')
        self.assertIsNone(self.parentBlock.findInst('i3'))
        #net
        self.assertEqual(self.block.findNet('n2').getName(), 'n2')
        self.assertIsNone(self.block.findNet('a'))
        #iterm
        self.assertEqual(self.block.findITerm('i1,o').getInst().getName(), 'i1')
        self.assertEqual(self.block.findITerm('i1,o').getMTerm().getName(), 'o')
        self.assertIsNone(self.block.findITerm('i1\o'))
        #extcornerblock
        self.assertEqual(self.block.findExtCornerBlock(1).getName(), 'extCornerBlock__1')
        self.assertIsNone(self.block.findExtCornerBlock(0))
        #nondefaultrule
        self.assertEqual(self.block.findNonDefaultRule('non_default_1').getName(), 'non_default_1')
        self.assertIsNone(self.block.findNonDefaultRule('non_default_2'))
        #region
        self.assertEqual(self.block.findRegion('parentRegion').getName(), 'parentRegion')
        self.assertEqual(self.block.findRegion('childRegion').getName(), 'childRegion')
        self.assertEqual(self.block.findRegion('childRegion').getParent().getName(), 'parentRegion')
        
        
    def check_box_rect(self, min_x, min_y, max_x, max_y):
        box = self.block.getBBox()
        self.assertEqual(box.xMin(), min_x)
        self.assertEqual(box.xMax(), max_x)
        self.assertEqual(box.yMin(), min_y)
        self.assertEqual(box.yMax(), max_y)
        
    def block_placement(self, test_num, flag):
        if( (flag and test_num==1) or (not flag and test_num>=1) ):
            if(flag):
                print("here")
            placeInst(self.block.findInst('i1'), 0, 3000)
            placeInst(self.block.findInst('i2'), -1000, 0)
            placeInst(self.block.findInst('i3'), 2000, -1000)
        if((flag and test_num==2) or (not flag and test_num>=2)):
            placeBPin(self.block.findBTerm('OUT').getBPins()[0], self.lib.getTech().findLayer('L1'), 2500, -1000, 2550, -950)
        if((flag and test_num==3) or (not flag and test_num>=3)):
            odb.dbObstruction_create(self.block, self.lib.getTech().findLayer('L1'), -1500, 0, -1580, 50)
        if((flag and test_num==4) or (not flag and test_num>=4)):
            n_s = odb.dbNet_create(self.block, 'n_s')
            swire = odb.dbSWire_create(n_s, 'NONE')
            odb.dbSBox_create(swire, self.lib.getTech().findLayer('L1'), 0, 4000, 100, 4100, 'NONE')
        if((flag and test_num==5) or (not flag and test_num>=5)):
            pass
            #TODO ADD WIRE
             
    def test_bbox0(self):
        box = self.block.getBBox()
        self.check_box_rect(0, 0, 0, 0)
    def test_bbox1(self):
        box = self.block.getBBox()
        self.block_placement(1,False)
        self.check_box_rect(-1000, -1000, 2500, 4000)
    def test_bbox2(self):
        box = self.block.getBBox()
        self.block_placement(2,False)
        self.check_box_rect(-1000, -1000, 2550, 4000)
    def test_bbox3(self):
#         self.block_placement(2,False)
#         box = self.block.getBBox()
#         self.block_placement(3,True)
        placeInst(self.block.findInst('i1'), 0, 3000)
        placeInst(self.block.findInst('i2'), -1000, 0)
        placeInst(self.block.findInst('i3'), 2000, -1000)
        placeBPin(self.block.findBTerm('OUT').getBPins()[0], self.lib.getTech().findLayer('L1'), 2500, -1000, 2550, -950)
        box = self.block.getBBox()
        odb.dbObstruction_create(self.block, self.lib.getTech().findLayer('L1'), -1500, 0, -1580, 50)
        self.check_box_rect(-1580, -1000, 2550, 4000)
    def test_bbox4(self):
        box = self.block.getBBox()
        self.block_placement(4,False)
        self.check_box_rect(-1580, -1000, 2550, 4100)
if __name__=='__main__':
    odbUnitTest.mainParallel(TestBlock)
#     odbUnitTest.main()
    
