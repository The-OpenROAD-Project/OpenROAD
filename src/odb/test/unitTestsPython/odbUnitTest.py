import unittest
class TestCase(unittest.TestCase):
    #Function to change a value and test the change effect
    def changeAndTest(self, obj, SetterName, GetterName, expectedVal, *args):
        getattr(obj,  SetterName)(*args)
        self.assertEqual(getattr(obj,  GetterName)(), expectedVal)
    def check(self, obj, GetterName, expectedVal, *args):
        self.assertEqual(getattr(obj,  GetterName)(*args), expectedVal)
    def change(self, obj, SetterName, *args):
        return getattr(obj,  SetterName)(*args)

def main():
    unittest.main()