import argparse
import io
from unittest import TestCase
from unittest.mock import patch, MagicMock

import whittle

default_args = argparse.Namespace(
    persistence=4,
    use_stdout=True,
    error_string="Iter: 100",
    base_db_path="/dummy/path/base.db",
    step="dummy_step",
    timeout=100,
    multiplier=1,
    exit_early_on_error=True,
    dump_def=False,
)


# ---------------------------------------------------------------------------
# Mock helpers – simulate the in-memory element lists that the real TCL
# scripts would operate on.
# ---------------------------------------------------------------------------


def _make_mock_perform_step(debugger):
    """Return a perform_step replacement that operates on in-memory lists."""

    def mock_perform_step(mock_self, cut_index=-1):
        before_cuts = mock_self.get_cuts()

        if cut_index == -1:
            return mock_self.error_string, before_cuts

        # Simulate cut_db: remove a slice of elements.
        num_elms = mock_self.get_num_elms()
        cuts = mock_self.get_cuts()
        start = num_elms * cut_index // cuts
        end = num_elms * (cut_index + 1) // cuts

        # Save state so we can roll back if the error disappears.
        mock_self._saved_insts = list(mock_self._insts)
        mock_self._saved_nets = list(mock_self._nets)

        if mock_self.cut_level == whittle.CutLevel.Insts:
            del mock_self._insts[start:end]
            mock_self.num_insts = len(mock_self._insts)
        else:
            del mock_self._nets[start:end]
            mock_self.num_nets = len(mock_self._nets)

        after_cuts = mock_self.get_cuts()

        if mock_self.check_error(mock_self._insts, mock_self._nets):
            return mock_self.error_string, after_cuts

        # Roll back – error not reproduced with this cut.
        mock_self._insts = mock_self._saved_insts
        mock_self._nets = mock_self._saved_nets
        mock_self.num_insts = len(mock_self._insts)
        mock_self.num_nets = len(mock_self._nets)
        return None, before_cuts

    return mock_perform_step


class TestWhittle(TestCase):
    @patch("os.path.exists", lambda x: True)
    def setUp(self):
        self.perform_step_patch = patch.object(
            whittle.Whittler, "perform_step", autospec=True
        )
        self.prepare_new_step_patch = patch.object(
            whittle.Whittler, "prepare_new_step", autospec=True
        )
        self.read_counts_patch = patch.object(
            whittle.Whittler, "read_counts", autospec=True
        )
        self.cleanup_db_patch = patch.object(
            whittle.Whittler, "cleanup_db", autospec=True
        )
        self.shutil_copy_patch = patch("shutil.copy")
        self.os_rename_patch = patch("os.rename")

        self.mock_perform_step = self.perform_step_patch.start()
        self.mock_prepare_new_step = self.prepare_new_step_patch.start()
        self.mock_read_counts = self.read_counts_patch.start()
        self.mock_cleanup_db = self.cleanup_db_patch.start()
        self.mock_shutil_copy = self.shutil_copy_patch.start()
        self.mock_os_rename = self.os_rename_patch.start()

        self.whittler = whittle.Whittler(default_args)

    def tearDown(self):
        self.perform_step_patch.stop()
        self.prepare_new_step_patch.stop()
        self.read_counts_patch.stop()
        self.cleanup_db_patch.stop()
        self.shutil_copy_patch.stop()
        self.os_rename_patch.stop()

    def _setup_run(self, num_insts, num_nets, check_error, persistence=4):
        """Wire up mock lists and the error-checking predicate."""
        self.whittler._insts = list(range(num_insts))
        self.whittler._nets = list(range(num_nets))
        self.whittler.num_insts = num_insts
        self.whittler.num_nets = num_nets
        self.whittler.check_error = check_error
        self.whittler.persistence = persistence

        self.mock_perform_step.side_effect = _make_mock_perform_step(self.whittler)
        self.mock_prepare_new_step.side_effect = lambda self_: None

    # ------------------------------------------------------------------
    # Test cases
    # ------------------------------------------------------------------

    def test_pruned_to_one(self):
        error_insts = [5]
        error_nets = [7]

        def check_error(insts, nets):
            return any(x in error_insts for x in insts) and any(
                x in error_nets for x in nets
            )

        self._setup_run(10, 10, check_error)
        self.whittler.debug()
        self.assertEqual(self.whittler._insts, error_insts)
        self.assertEqual(self.whittler._nets, error_nets)

    def test_lots_down_to_one(self):
        error_insts = set(range(50000, 50010))
        error_nets = set(range(70000, 80000))

        def check_error(insts, nets):
            return any(x in error_insts for x in insts) and any(
                x in error_nets for x in nets
            )

        self._setup_run(100000, 100000, check_error)
        self.whittler.debug()
        self.assertEqual(len(self.whittler._insts), 1)
        self.assertEqual(len(self.whittler._nets), 1)

    def test_minimum_errors(self):
        error_insts = set(range(500, 601))
        error_nets = set(range(700, 751))

        def check_error(insts, nets):
            return (
                any(x in error_insts for x in insts)
                and len(insts) >= 100
                and any(x in error_nets for x in nets)
                and len(nets) >= 75
            )

        self._setup_run(1000, 1000, check_error, persistence=6)
        self.whittler.debug()
        self.assertEqual(len(self.whittler._insts), 100)
        self.assertEqual(len(self.whittler._nets), 75)

    def test_all_in_a_range(self):
        error_insts = set(range(50, 60))
        error_nets = set(range(70, 90))

        def check_error(insts, nets):
            return all(x in insts for x in error_insts) and all(
                x in nets for x in error_nets
            )

        self._setup_run(100, 100, check_error, persistence=6)
        self.whittler.debug()
        self.assertEqual(set(self.whittler._insts), error_insts)
        self.assertEqual(set(self.whittler._nets), error_nets)

    def test_no_insts(self):
        error_nets = set(range(70, 90))

        def check_error(insts, nets):
            return len(insts) >= 0 and all(x in nets for x in error_nets)

        self._setup_run(100, 100, check_error, persistence=6)
        self.whittler.debug()
        self.assertEqual(len(self.whittler._insts), 0)
        self.assertEqual(set(self.whittler._nets), error_nets)

    def test_no_nets(self):
        error_insts = set(range(70, 90))

        def check_error(insts, nets):
            return len(nets) >= 0 and all(x in insts for x in error_insts)

        self._setup_run(100, 100, check_error, persistence=6)
        self.whittler.debug()
        self.assertEqual(set(self.whittler._insts), error_insts)
        self.assertEqual(len(self.whittler._nets), 0)

    def test_progress_output(self):
        error_insts = [5]
        error_nets = [7]

        def check_error(insts, nets):
            return any(x in error_insts for x in insts) and any(
                x in error_nets for x in nets
            )

        self._setup_run(10, 10, check_error)

        captured = io.StringIO()
        with patch("sys.stdout", captured):
            self.whittler.debug()

        output = captured.getvalue()
        self.assertIn("[whittle]", output)
        self.assertIn("Phase: Insts", output)
        self.assertIn("Phase: Nets", output)
        self.assertIn("Elapsed:", output)
        self.assertIn("Whittling Done!", output)

    def test_format_duration(self):
        self.assertEqual(whittle.Whittler.format_duration(30), "30s")
        self.assertEqual(whittle.Whittler.format_duration(90), "1m")
        self.assertEqual(whittle.Whittler.format_duration(120), "2m")
        self.assertEqual(whittle.Whittler.format_duration(3661), "1h 01m")
        self.assertEqual(whittle.Whittler.format_duration(7200), "2h 00m")


class TestWhittleTclInterface(TestCase):
    """Test the subprocess/TCL interface without algorithm mocks."""

    @patch("os.path.exists", lambda x: True)
    def setUp(self):
        self.whittler = whittle.Whittler(default_args)

    def test_parse_counts(self):
        w = self.whittler
        w._parse_counts("INSTS 42\nNETS 17\n")
        self.assertEqual(w.num_insts, 42)
        self.assertEqual(w.num_nets, 17)

    def test_parse_counts_with_extra_output(self):
        w = self.whittler
        w._parse_counts("OpenROAD v2.0\nINSTS 100\nfoo bar\nNETS 200\n")
        self.assertEqual(w.num_insts, 100)
        self.assertEqual(w.num_nets, 200)

    @patch("subprocess.run")
    def test_cut_db_calls_openroad(self, mock_run):
        mock_run.return_value = MagicMock(returncode=0, stdout="INSTS 5\nNETS 10\n")
        self.whittler.cut_db("/in.odb", "/out.odb", whittle.CutLevel.Insts, 0, 3)
        mock_run.assert_called_once()
        args = mock_run.call_args
        cmd = args[0][0]
        self.assertEqual(cmd[0], self.whittler.openroad_exe)
        self.assertIn("-exit", cmd)
        env = args[1]["env"]
        self.assertEqual(env["WHITTLE_CUT_LEVEL"], "insts")
        self.assertEqual(env["WHITTLE_CUT_START"], "0")
        self.assertEqual(env["WHITTLE_CUT_END"], "3")
        self.assertEqual(self.whittler.num_insts, 5)
        self.assertEqual(self.whittler.num_nets, 10)

    @patch("subprocess.run")
    def test_cleanup_db_calls_openroad(self, mock_run):
        mock_run.return_value = MagicMock(returncode=0, stdout="REMOVED 12\n")
        self.whittler.cleanup_db("/in.odb", "/out.odb")
        mock_run.assert_called_once()
        env = mock_run.call_args[1]["env"]
        self.assertEqual(env["WHITTLE_DB_INPUT"], "/in.odb")
        self.assertEqual(env["WHITTLE_DB_OUTPUT"], "/out.odb")
