#!/usr/bin/env python3

import os
import json
import unittest
import subprocess


class CredHelperTests(unittest.TestCase):
    """Tests the cred_helper.py script requirements"""

    def setUp(self):
        """Sets up unit tests by defining location of script"""
        dir_name = os.path.dirname(__file__)
        self._exe = os.path.join(dir_name, "cred_helper.py")

    def test_test_mode(self):
        """
        Tests that the format of the test mode output is correct and that
        the user in the user.bazelrc has the correct entitlements
        """
        result = subprocess.run(
            [self._exe, "test"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        self.assertEqual(
            result.returncode,
            0,
            "Test op returned non-zero status. Check user account defined in user.bazelrc",
        )
        self.assertEqual("", result.stderr)
        try:
            # The first line is a debug message that has the user name
            loc = result.stdout.index("\n")
            debug_msg = result.stdout[:loc]
            user_name = debug_msg.split()[-1]
            result.stdout = result.stdout[loc + 1 :]
            json_data = json.loads(result.stdout)
            self.assertTrue(
                "storage.buckets.get" in json_data["permissions"],
                "{} is missing read access".format(user_name),
            )
            self.assertTrue(
                "storage.objects.create" in json_data["permissions"],
                "{} is missing write access".format(user_name),
            )
        except json.decoder.JSONDecodeError as e:
            self.assertFalse("JSON had syntax errors: {}".format(e))

    def test_get_mode(self):
        """
        Tests that the format of the get mode output is correct and that
        the user in the user.bazelrc has the correct entitlements
        """
        result = subprocess.run(
            [self._exe, "get"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        self.assertEqual(
            result.returncode,
            0,
            "Get op returned non-zero status. Check user account defined in user.bazelrc",
        )
        self.assertEqual("", result.stderr)
        try:
            json_data = json.loads(result.stdout)
            self.assertIsInstance(
                json_data["headers"]["Authorization"],
                list,
                "Authorization is expected to be a list",
            )
        except json.decoder.JSONDecodeError as e:
            self.assertFalse("JSON had syntax errors: {}".format(e))

    def test_usage_mode(self):
        """
        Tests that the format of calling the script without arguments output is
        correct and the exit code is non-zero
        """
        result = subprocess.run(
            [self._exe], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
        )
        # return code is supposed to be non-zero
        self.assertEqual(result.returncode, 1)
        # messages go to stderr, not stdout
        self.assertEqual("", result.stdout)
        self.assertTrue(result.stderr.startswith("Usage: "))

    def test_extra_args(self):
        """
        Tests that we generate a syntax error when calling with more than two
        args
        """
        result = subprocess.run(
            [self._exe, "1", "2"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        # return code is supposed to be non-zero
        self.assertEqual(result.returncode, 1)
        # messages go to stderr, not stdout
        self.assertEqual("", result.stdout)
        self.assertTrue(result.stderr.startswith("Usage: "))


if __name__ == "__main__":
    unittest.main()
