import argparse
import sys
from unittest import TestCase
from unittest.mock import patch, MagicMock

odb = MagicMock()
sys.modules["odb"] = odb
openroad = MagicMock()
sys.modules["openroad"] = openroad
import deltaDebug

default_args = argparse.Namespace(
    persistence=4,
    use_stdout=True,
    error_string="Iter: 100",
    base_db_path="/dummy/path/base.db",
    base_db_file=None,
    original_db_file=None,
    step="dummy_step",
    timeout=100,
    multiplier=1,
    exit_early_on_error=True,
    dump_def=True,
)


def mock_get_insts(mock_self):
    return mock_self.insts


def mock_get_nets(mock_self):
    return mock_self.nets


def mock_perform_step(mock_self, cut_index=-1):
    before_cuts = mock_self.get_cuts()

    if cut_index == -1:
        return mock_self.error_string, before_cuts
    mock_self.cut_block(index=cut_index)

    after_cuts = mock_self.get_cuts()
    if mock_self.check_error(mock_self.insts, mock_self.nets):
        print(f"Error Code found: {mock_self.error_string}")
        return mock_self.error_string, after_cuts

    mock_self.insts = list(mock_self.insts_saved)
    mock_self.nets = list(mock_self.nets_saved)
    return None, before_cuts


def mock_cut_elements(mock_self, start, end):
    mock_self.insts_saved = list(mock_self.insts)
    mock_self.nets_saved = list(mock_self.nets)
    elements = (
        mock_self.insts
        if mock_self.cut_level == deltaDebug.cutLevel.Insts
        else mock_self.nets
    )
    del elements[start:end]


def mock_prepare_new_step(mock_self):
    pass


class TestDeltaDebug(TestCase):
    @patch("os.path.exists", lambda x: True)
    def setUp(self):
        args = default_args

        self.perform_step_patch = patch.object(
            deltaDebug.deltaDebugger,
            "perform_step",
            wraps=mock_perform_step,
            autospec=True,
        )
        self.get_insts_patch = patch.object(
            deltaDebug.deltaDebugger, "get_insts", autospec=True
        )
        self.get_nets_patch = patch.object(
            deltaDebug.deltaDebugger, "get_nets", autospec=True
        )
        self.cut_elements = patch.object(
            deltaDebug.deltaDebugger, "cut_elements", autospec=True
        )
        self.prepare_new_step = patch.object(
            deltaDebug.deltaDebugger, "prepare_new_step", autospec=True
        )
        self.shutil_copy_patch = patch("shutil.copy")
        self.os_rename_patch = patch("os.rename")

        self.mock_perform_step = self.perform_step_patch.start()
        self.mock_get_insts = self.get_insts_patch.start()
        self.mock_get_nets = self.get_nets_patch.start()
        self.mock_shutil_copy = self.shutil_copy_patch.start()
        self.mock_os_rename = self.os_rename_patch.start()
        self.mock_cut_elements = self.cut_elements.start()
        self.mock_prepare_new_step = self.prepare_new_step.start()

        self.mock_perform_step.side_effect = mock_perform_step
        self.mock_get_insts.side_effect = mock_get_insts
        self.mock_get_nets.side_effect = mock_get_nets
        self.mock_cut_elements.side_effect = mock_cut_elements

        self.debugger = deltaDebug.deltaDebugger(args)

    def tearDown(self):
        self.perform_step_patch.stop()
        self.get_insts_patch.stop()
        self.get_nets_patch.stop()
        self.shutil_copy_patch.stop()
        self.os_rename_patch.stop()
        self.cut_elements.stop()
        self.prepare_new_step.stop()

    def test_pruned_to_one(self):
        self.debugger.insts = list(range(10))
        self.debugger.nets = list(range(10))
        error_insts = [5]
        error_nets = [7]

        def check_error(insts, nets):
            return any(map(lambda x: x in error_insts, insts)) and any(
                map(lambda x: x in error_nets, nets)
            )

        self.debugger.check_error = check_error
        self.debugger.persistence = 4
        self.debugger.debug()
        self.assertEqual(self.debugger.insts, error_insts)
        self.assertEqual(self.debugger.nets, error_nets)

    def test_lots_down_to_one(self):
        self.debugger.insts = list(range(100000))
        self.debugger.nets = list(range(100000))
        error_insts = set(range(50000, 50010))
        error_nets = set(range(70000, 80000))

        def check_error(insts, nets):
            return any(map(lambda x: x in error_insts, insts)) and any(
                map(lambda x: x in error_nets, nets)
            )

        self.debugger.check_error = check_error
        self.debugger.persistence = 4
        self.debugger.debug()
        self.assertEqual(len(self.debugger.insts), 1)
        self.assertEqual(len(self.debugger.nets), 1)

    def test_minimum_errors(self):
        self.debugger.insts = list(range(1000))
        self.debugger.nets = list(range(1000))
        error_insts = set(range(500, 601))
        error_nets = set(range(700, 751))

        def check_error(insts, nets):
            return (
                any(map(lambda x: x in error_insts, insts))
                and len(insts) >= 100
                and any(map(lambda x: x in error_nets, nets))
                and len(nets) >= 75
            )

        self.debugger.check_error = check_error
        self.debugger.persistence = 6
        self.debugger.debug()
        self.assertEqual(len(self.debugger.insts), 100)
        self.assertEqual(len(self.debugger.nets), 75)

    def test_all_in_a_range(self):
        self.debugger.insts = list(range(100))
        self.debugger.nets = list(range(100))
        error_insts = set(range(50, 60))
        error_nets = set(range(70, 90))

        def check_error(insts, nets):
            return all(map(lambda x: x in insts, error_insts)) and all(
                map(lambda x: x in nets, error_nets)
            )

        self.debugger.check_error = check_error
        self.debugger.persistence = 6
        self.debugger.debug()
        self.assertEqual(set(self.debugger.insts), set(error_insts))
        self.assertEqual(set(self.debugger.nets), set(error_nets))

    def test_no_insts(self):
        self.debugger.insts = list(range(100))
        self.debugger.nets = list(range(100))
        error_nets = set(range(70, 90))

        def check_error(insts, nets):
            return len(insts) >= 0 and all(map(lambda x: x in nets, error_nets))

        self.debugger.check_error = check_error
        self.debugger.persistence = 6
        self.debugger.debug()
        self.assertEqual(len(self.debugger.insts), 0)
        self.assertEqual(set(self.debugger.nets), set(error_nets))

    def test_no_nets(self):
        self.debugger.insts = list(range(100))
        self.debugger.nets = list(range(100))
        error_insts = set(range(70, 90))

        def check_error(insts, nets):
            return len(nets) >= 0 and all(map(lambda x: x in insts, error_insts))

        self.debugger.check_error = check_error
        self.debugger.persistence = 6
        self.debugger.debug()
        self.assertEqual(set(self.debugger.insts), set(error_insts))
        self.assertEqual(len(self.debugger.nets), 0)
