import opendbpy as odb
import helper
import odbUnitTest
##################
##
## This test is intended for testing swig wrap
##
##################
class TestModule(odbUnitTest.TestCase):
    def setUp(self):
        self.db, self.lib   = helper.createSimpleDB()
        self.block          = helper.create1LevelBlock(self.db, self.lib, self.db.getChip())
        self.group          = odb.dbGroup_create(self.block,"group")
        self.domain         = odb.dbGroup_create(self.block,"domain",0,0,100,100)
        self.child          = odb.dbGroup_create(self.block,"child")
        self.master_mod     = odb.dbModule_create(self.block,"master_mod")
        self.parent_mod     = odb.dbModule_create(self.block,"parent_mod")
        self.i1             = odb.dbModInst_create(self.parent_mod,self.master_mod,"i1")
        self.inst1          = self.block.findInst("inst")
        self.n1             = self.block.findNet("n1")
    def tearDown(self):
        self.db.destroy(self.db)
    def test_default(self):
        self.check(self.group,"getName","group")
        print("")
        self.assertEqual(self.block.findGroup("group").getName(),"group")
        self.assertEqual(len(self.block.getGroups()),3)
        self.assertEqual(self.domain.getBox().xMax(),100)
        self.group.addModInst(self.i1)
        self.assertEqual(self.group.getModInsts()[0].getName(),"i1")
        self.assertEqual(self.i1.getGroup().getName(),"group")
        self.group.removeModInst(self.i1)
        self.group.addInst(self.inst1)
        self.assertEqual(self.group.getInsts()[0].getName(),"inst")
        self.assertEqual(self.inst1.getGroup().getName(),"group")
        self.group.removeInst(self.inst1)
        self.group.addGroup(self.child)
        self.assertEqual(self.group.getGroups()[0].getName(),"child")
        self.assertEqual(self.child.getParentGroup().getName(),"group")
        self.group.removeGroup(self.child)
        self.group.addPowerNet(self.n1)
        self.assertEqual(self.group.getPowerNets()[0].getName(),"n1")
        self.group.addGroundNet(self.n1)
        self.assertEqual(self.group.getGroundNets()[0].getName(),"n1")
        self.group.removeNet(self.n1)
        self.assertEqual(self.group.getType(),"PHYSICAL_CLUSTER")
        self.group.setType("VOLTAGE_DOMAIN")
        self.group.destroy(self.group)
if __name__=='__main__':
    odbUnitTest.mainParallel(TestModule)     
