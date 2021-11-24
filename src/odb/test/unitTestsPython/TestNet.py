import opendbpy as odb
import helper
import odbUnitTest

#TestNet: A unit test class for class dbNet
#it inherits from odbunittest.TestCase and has access to the testing functions(asserts)
class TestNet(odbUnitTest.TestCase):
    #This Function is called before each of the test cases defined below
    #You should use it to create the instances you need to test (in our case n1, n2, n3)
    def setUp(self):
        self.db, lib = helper.createSimpleDB()
        block = helper.create1LevelBlock(self.db, lib, self.db.getChip())
        inst = block.getInsts()[0]
        self.n1 = inst.findITerm('a').getNet()
        self.n2 = inst.findITerm('b').getNet()
        self.n3 = inst.findITerm('o').getNet()
    #this function is called after each of the test cases
    #you should free up space and destroy unneeded objects(cleanup step)
    def tearDown(self):
        self.db.destroy(self.db)
        
    #each test case should start with the name "test"
    def test_naming(self):
        self.changeAndTest(self.n1, 'rename', 'getName', '_n1', '_n1')
        self.check(self.n1, 'getConstName', '_n1')
        self.assertFalse(self.change(self.n1, 'rename', 'n2'))
    def test_dbSetterAndGetter(self):
        self.changeAndTest(self.n1, 'setRCDisconnected', 'isRCDisconnected', False, False)
        self.changeAndTest(self.n1, 'setRCDisconnected', 'isRCDisconnected', True, True)
        self.changeAndTest(self.n1, 'setWeight', 'getWeight', 2, 2)
        self.changeAndTest(self.n1, 'setSourceType', 'getSourceType', 'NETLIST', 'NETLIST')
        self.changeAndTest(self.n1, 'setXTalkClass', 'getXTalkClass', 1, 1)
        self.changeAndTest(self.n1, 'setCcAdjustFactor', 'getCcAdjustFactor', 1, 1)
        self.changeAndTest(self.n1, 'setSigType', 'getSigType', 'RESET', 'RESET')
    def test_dbCc(self):
        self.changeAndTest(self.n1, 'setDbCc', 'getDbCc', 2, 2)
        self.changeAndTest(self.n1, 'addDbCc', 'getDbCc', 5, 3)
    def test_cc(self):
        node2 = odb.dbCapNode_create(self.n2, 0, False)
        node1 = odb.dbCapNode_create(self.n1, 1, False)
        node1.setInternalFlag()
        ccseg = odb.dbCCSeg_create(node1, node2)
        self.n1.calibrateCouplingCap()
        self.check(self.n1, 'maxInternalCapNum', 1)
        self.check(self.n1, 'groundCC', True, 1)
        self.check(self.n2, 'groundCC', False, 1)
        self.check(self.n1, 'getCcCount', 1)
if __name__=='__main__':
    odbUnitTest.mainParallel(TestNet)