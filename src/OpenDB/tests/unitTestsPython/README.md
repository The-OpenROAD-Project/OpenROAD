# Python Unit Tests

### Running tests

For running the python unit tests you will need to install first *testtools* and *unittest-parallel* which enables concurrent unit testing

```shell
pip3 install testtools
pip3 install unittest-parallel
```

Then, you can run the unit tests  in sequence by running

```shell
./../unitTests.sh
```

or in parallel by running

```shell
./../unitTests.sh parallel
```

**Note**: The test cases within each Unit Test run in parallel in both situations

--------------------------------------------------------------------------

### <u>Tests Structure</u>

The directory unitTestsPython includes unit tests for OpenDB Python APIs. Any test file starts with 'Test' followed by the test target.

#### odbUnitTest.py:

This includes `TestCase` class which inherits from `unittest.TestCase` with additional functionalities:

* `changeAndTest(self,obj,SetterName,GetterName,expectedVal,*args)`which is a function for changing a value and testing for the effect of that change where:

  * `obj` is the object to be tested
  * `SetterName` is the name of the function to be called for changing a value
  * `GetterName` is the name of the function to be called for testing the effect
  * `expectedVal` is the expected value for the testing
  * `*args` are the arguments passed to the `SetterName` function

  So, in the end, the expected behavior is:

  ```python
  obj.SetterName(*args)
  
  assert(obj.GetterName()==expectedVal)`
  ```

  

* `check(self,obj,GetterName,expectedVal,*args)` which tests against expected value

* `change(self,obj,SetterName,*args)` which changes a value in the object

* `main()` runs the `TestCase` in sequential order

* `mainParallel(Test)` runs the passed `Test` class in parallel

#### helper.py:

A set of functions for creating simple db instances to be used for testing.  You can find the description of each function in the comments

#### TestNet.py:

Unit test class for testing dbNet. It inherits from `odbUnitTest.TestCase` . it consists of

* `setUp(self)` function to be called before each test case. Here, we create the database with the desired chip, block, masters, instances and nets.
* `tearDown(self)` function to be called after each test case. Here, we destroy our db.
* `test_*(self)` the test cases functions. Their names should start with `test` for the unittest suite to recognize. 

#### TestDestroy.py:

Integration test class for testing the `destroy(*args)` function on OpenDB.

* `test_destroy_net` destroying net and testing for the effect on the *block,inst, iterms and bterms*
* `test_destroy_inst` destroying instance and testing for the effect on *block, iterms, net, bterms*
* `test_destroy_bterm` destroying bterm and testing for the effect on *block and net*
* `test_destroy_block` destroying block and testing for the effect on *block(parent and child relation), and chip*
* `test_destroy_bpin` destroying bpin and testing for the effect on *bterm*
* `test_create_destroy_wire` destroying wire and test for the effect on *net*
* `test_destroy_capnode` destroying capnode and test for the effect on *net(node and connected ccsegs)*
* `test_destroy_ccseg` destroying ccseg and test for the effect on *node,block and net*

* `test_destroy_lib` destroying lib and test for the effect on *db*

* `test_destroy_obstruction` destroying obstruction and test for the effect on *block*
* `test_create_regions` creating regions and test for the effect on *block and region(parent and child relation)*
* `test_destroy_region_child` destroying _ and test for the effect on *block and region(parent)*

* `test_destroy_region_parent` destroying _ and test for the effect on *block*

#### TestBlock.py:

Unit Test for dbBlock

* `test_find` testing the find function with *BTerm, Child, Inst, Net, ITerm, ExtCornerBlock, nonDefaultRule, Region*

* Testing the ComputeBBox() function through the first call of getBBox:
  * `test_bbox0` testing empty block box
  * `test_bbox1` testing block box with Inst placed
  * `test_bbox2` testing block box with Inst and BPin placed
  * `test_bbox3` testing block box with Inst, BPin and Obstruction placed
  * `test_bbox3` testing block box with Inst, BPin, Obstruction and SWire placed

#### TestBTerm.py:

Unit Test for dbBTerm

* `test_idle` testing for idle disconnected  `BTerm` behavior
* `test_connect` testing connect function of `BTerm` on `BTerm` and `Net`
* `test_disconnect` testing disconnect function of `BTerm` on `BTerm` and `Net`

#### TestInst.py:

Unit Test for dbInst

* `test_swap_master` testing swap master function

#### TestITerm.py:

Unit Test for dbITerm

* `test_idle` testing for disconnected ITerm without a net
* `test_connection_from_iterm` testing the connect(ITerm,...) and disconnect functions of ITerm and their effect on ITerm and Net
* `test_connection_from_inst` testing the connect(Inst,...) and disconnect functions of ITerm and their effect on ITerm and Net
* Testing for getAvgXY() function
  * `test_avgxy_R0` testing with default orientation R0
  * `test_avgxy_R90` testing with different orientation R90 for transformation

--------------------------

##### Problems Found In Testing

* multiple core dumps that leads to aborting the process:
  * dbNet.get1st*()			(when nothing on top of the list)
  * childRegion.getParent()       (after destroying the parent region)
* Implementation of ComputeBBox() is flawed and needs to be reconsidered

