"""Delta-debugging tool for minimising OpenROAD .odb test cases.

Unlike deltaDebug.py this script does *not* require Python to be compiled
into OpenROAD.  Database manipulation is delegated to small TCL scripts
(whittle_cut.tcl, whittle_cleanup.tcl) that are executed via
``openroad -exit``.  All imports are from the Python standard library, so
any system ``python3`` works with no extra packages.

Typical invocation through Bazel (sets ``OPENROAD_EXE`` and ``PATH``
automatically)::

    bazelisk run //:whittle -- \\
        --base_db_path foo.odb \\
        --error_string "GPL-0305" \\
        --step "make --file=$FLOW_HOME/Makefile do-3_3_place_gp"

With ``openroad`` on ``PATH`` (e.g. after ``bazelisk run //:install`` and
``source env.sh`` in ORFS) you can also invoke whittle directly with the
system Python interpreter.  If ``openroad`` is not on ``PATH``, place it
(or a symlink) next to ``whittle.py`` and it will be found automatically::

    python3 etc/whittle.py \\
        --base_db_path foo.odb \\
        --error_string "GPL-0305" \\
        --step "make --file=$FLOW_HOME/Makefile do-3_3_place_gp"
"""

import argparse
import enum
import errno
import os
import re
import select
import shutil
import signal
import subprocess
import sys
import time
from math import ceil

PERSISTENCE_RANGE = range(1, 7)
CUT_MULTIPLE = range(1, 128)


class CutLevel(enum.Enum):
    Insts = 1
    Nets = 0


def _build_parser():
    parser = argparse.ArgumentParser(
        description="Delta-debug / whittle down an OpenROAD .odb file"
    )
    parser.add_argument(
        "--base_db_path",
        type=str,
        required=True,
        help="Path to the .odb file to minimise",
    )
    parser.add_argument(
        "--error_string",
        type=str,
        required=True,
        help="Output substring that indicates the target error",
    )
    parser.add_argument(
        "--step",
        type=str,
        required=True,
        help="Shell command to run on the .odb (e.g. a make target)",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=None,
        help="Initial timeout in seconds (default: auto-measured)",
    )
    parser.add_argument(
        "--multiplier",
        type=int,
        default=1,
        choices=CUT_MULTIPLE,
        help="Multiply number of cuts",
    )
    parser.add_argument(
        "--persistence",
        type=int,
        default=1,
        choices=PERSISTENCE_RANGE,
        help="Max granularity: fragments = 2^persistence",
    )
    parser.add_argument(
        "--use_stdout",
        action="store_true",
        help="Also search stdout for the error string",
    )
    parser.add_argument(
        "--exit_early_on_error",
        action="store_true",
        help="Abort a step early when an unrelated ERROR appears",
    )
    parser.add_argument(
        "--dump_def",
        action="store_true",
        help="Write DEF files alongside ODB at each step",
    )
    return parser


class Whittler:
    """Iteratively removes instances and nets from an .odb until the error
    string can no longer be reproduced, yielding a minimal test case."""

    def __init__(self, opt):
        if not os.path.exists(opt.base_db_path):
            raise FileNotFoundError(
                errno.ENOENT, os.strerror(errno.ENOENT), opt.base_db_path
            )

        base_db_directory = os.path.dirname(opt.base_db_path) or "."
        base_db_name = os.path.basename(opt.base_db_path)

        self.base_db_file = opt.base_db_path
        self.error_string = opt.error_string
        self.use_stdout = opt.use_stdout
        self.exit_early_on_error = opt.exit_early_on_error
        self.step_count = 1
        self.persistence = opt.persistence
        self.multiplier = opt.multiplier
        self.timeout = opt.timeout
        self.dump_def = opt.dump_def
        self.step = opt.step

        self.original_base_db_file = os.path.join(
            base_db_directory, f"deltaDebug_base_original_{base_db_name}"
        )
        self.temp_base_db_file = os.path.join(
            base_db_directory, f"deltaDebug_base_temp_{base_db_name}"
        )
        self.deltaDebug_result_base_file = os.path.join(
            base_db_directory, f"deltaDebug_base_result_{base_db_name}"
        )

        _script_dir = os.path.dirname(os.path.abspath(__file__))
        _candidates = [
            os.path.join(_script_dir, "openroad"),  # next to script
            os.path.join(
                _script_dir, "..", "..", "install", "OpenROAD", "bin", "openroad"
            ),  # ORFS install tree: tools/OpenROAD/etc/ -> tools/install/OpenROAD/bin/
        ]
        _default = next((c for c in _candidates if os.path.isfile(c)), "openroad")
        self.openroad_exe = os.environ.get("OPENROAD_EXE", _default)
        self.tcl_dir = _script_dir

        self.cut_level = CutLevel.Insts

        # Current element counts, refreshed after each TCL invocation.
        self.num_insts = 0
        self.num_nets = 0

    # -----------------------------------------------------------------
    # TCL helpers – these shell out to ``openroad -exit``
    # -----------------------------------------------------------------

    def _run_openroad_tcl(self, tcl_script, extra_env):
        """Run a TCL script through openroad and return its stdout."""
        env = {**os.environ, **extra_env}
        result = subprocess.run(
            [self.openroad_exe, "-exit", tcl_script],
            env=env,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            print(f"openroad TCL error:\n{result.stderr}", file=sys.stderr)
            result.check_returncode()
        return result.stdout

    def _parse_counts(self, stdout):
        """Parse ``INSTS <n>`` / ``NETS <n>`` lines from TCL output."""
        for line in stdout.splitlines():
            m = re.match(r"^INSTS\s+(\d+)", line)
            if m:
                self.num_insts = int(m.group(1))
            m = re.match(r"^NETS\s+(\d+)", line)
            if m:
                self.num_nets = int(m.group(1))

    def cut_db(self, input_path, output_path, cut_level, start, end):
        """Cut elements [start, end) and write *output_path*."""
        level = "insts" if cut_level == CutLevel.Insts else "nets"
        extra_env = {
            "WHITTLE_DB_INPUT": input_path,
            "WHITTLE_DB_OUTPUT": output_path,
            "WHITTLE_CUT_LEVEL": level,
            "WHITTLE_CUT_START": str(start),
            "WHITTLE_CUT_END": str(end),
        }
        if self.dump_def:
            def_path = output_path.rsplit(".", 1)[0] + ".def"
            extra_env["WHITTLE_DEF_OUTPUT"] = def_path

        tcl = os.path.join(self.tcl_dir, "whittle_cut.tcl")
        stdout = self._run_openroad_tcl(tcl, extra_env)
        self._parse_counts(stdout)

    def cleanup_db(self, input_path, output_path):
        """Remove unused masters from *input_path*, write to *output_path*."""
        extra_env = {
            "WHITTLE_DB_INPUT": input_path,
            "WHITTLE_DB_OUTPUT": output_path,
        }
        if self.dump_def:
            def_path = output_path.rsplit(".", 1)[0] + ".def"
            extra_env["WHITTLE_DEF_OUTPUT"] = def_path

        tcl = os.path.join(self.tcl_dir, "whittle_cleanup.tcl")
        stdout = self._run_openroad_tcl(tcl, extra_env)
        m = re.search(r"REMOVED\s+(\d+)", stdout)
        if m:
            print(f"Removed {m.group(1)} unused masters.")

    def read_counts(self, db_path):
        """Read element counts from an ODB without modifying it."""
        extra_env = {
            "WHITTLE_DB_INPUT": db_path,
            "WHITTLE_DB_OUTPUT": db_path,
            "WHITTLE_CUT_LEVEL": "insts",
            "WHITTLE_CUT_START": "0",
            "WHITTLE_CUT_END": "0",
        }
        tcl = os.path.join(self.tcl_dir, "whittle_cut.tcl")
        stdout = self._run_openroad_tcl(tcl, extra_env)
        self._parse_counts(stdout)

    # -----------------------------------------------------------------
    # Step execution (runs the user-supplied command)
    # -----------------------------------------------------------------

    def run_command(self, command):
        """Run *command* in a shell, watching for *error_string*.

        Returns the error string if found, otherwise ``None``.
        """
        poll_obj = select.poll()
        if not self.use_stdout:
            process = subprocess.Popen(
                command,
                shell=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                encoding="utf-8",
                preexec_fn=os.setsid,
            )
            poll_obj.register(process.stderr, select.POLLIN)
        else:
            process = subprocess.Popen(
                command,
                shell=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                encoding="utf-8",
                preexec_fn=os.setsid,
            )
            poll_obj.register(process.stdout, select.POLLIN)

        start_time = time.time()
        try:
            return self._poll(process, poll_obj, start_time)
        finally:
            try:
                os.killpg(os.getpgid(process.pid), signal.SIGKILL)
            except ProcessLookupError:
                pass

    def _poll(self, process, poll_obj, start_time):
        error_string = None
        while True:
            if poll_obj.poll(1):
                if not self.use_stdout:
                    output = process.stderr.readline()
                else:
                    output = process.stdout.readline()

                if output.find(self.error_string) != -1:
                    error_string = self.error_string
                    break
                elif self.exit_early_on_error and output.find("ERROR") != -1:
                    break

            if (time.time() - start_time) > self.timeout:
                print(f"Step {self.step_count} timed out!", flush=True)
                break

            if process.poll() is not None:
                break

        return error_string

    # -----------------------------------------------------------------
    # Core delta-debugging loop
    # -----------------------------------------------------------------

    def get_num_elms(self):
        if self.cut_level == CutLevel.Insts:
            return self.num_insts
        return self.num_nets

    def get_cuts(self):
        return min(self.n * self.multiplier, self.get_num_elms())

    def perform_step(self, cut_index=-1):
        """Cut elements, run the step command, return (error_string, cuts)."""
        if cut_index != -1:
            num_elms = self.get_num_elms()
            cuts = self.get_cuts()
            start = num_elms * cut_index // cuts
            end = num_elms * (cut_index + 1) // cuts

            is_insts = self.cut_level == CutLevel.Insts
            level_name = "Insts" if is_insts else "Nets"
            message = [
                f"Step {self.step_count}",
                f"{level_name} level debugging",
                f"Insts {self.num_insts}",
                f"Nets {self.num_nets}",
                f"cut elements {end - start}",
                f"timeout {ceil(self.timeout / 60.0)} minutes",
            ]
            print(", ".join(message), flush=True)

            position = "#" * cuts
            ci = cut_index
            position = position[:ci] + "C" + position[ci + 1 :]
            print(f"[{position}]", flush=True)

            self.cut_db(
                self.temp_base_db_file,
                self.base_db_file,
                self.cut_level,
                start,
                end,
            )
        else:
            # No cut – just copy temp to base so the step command can read it.
            shutil.copy(self.temp_base_db_file, self.base_db_file)

        cuts = self.get_cuts() if cut_index != -1 else None

        start_time = time.time()
        error = self.run_command(self.step)
        end_time = time.time()

        if error is not None:
            self.timeout = max(120, 1.2 * (end_time - start_time))
            print(f"Error Code found: {error}")

        return error, cuts

    def prepare_new_step(self):
        """Promote the current base_db_file (a successful cut) to temp."""
        if os.path.exists(self.temp_base_db_file):
            os.remove(self.temp_base_db_file)
        if os.path.exists(self.base_db_file):
            os.rename(self.base_db_file, self.temp_base_db_file)
        # Refresh element counts from the new temp file.
        self.read_counts(self.temp_base_db_file)

    def debug(self):
        """Run the full delta-debugging algorithm."""
        print("Backing up original base file.")
        shutil.copy(self.base_db_file, self.original_base_db_file)
        os.rename(self.base_db_file, self.temp_base_db_file)

        # Read initial element counts.
        self.read_counts(self.temp_base_db_file)

        if self.timeout is None:
            self.timeout = 1e6
            print(
                "Performing a step with the original "
                "input file to calculate timeout."
            )
            error, _ = self.perform_step()
            if error is None:
                print("No error found in the original input file.")
                sys.exit(1)

        for self.cut_level in (CutLevel.Insts, CutLevel.Nets):
            while True:
                err = None
                self.n = 2

                cuts = None
                while self.n <= (2**self.persistence):
                    error_in_range = None
                    j = 0
                    while j == 0 or j < cuts:
                        current_err, cuts = self.perform_step(cut_index=j)
                        self.step_count += 1
                        if current_err is not None:
                            err = current_err
                            error_in_range = current_err
                            self.prepare_new_step()
                        j += 1

                    if error_in_range is None:
                        self.n *= 2
                    elif self.n >= 8:
                        self.n = int(self.n / 2)
                    else:
                        break

                if err is None or cuts == 0:
                    break

        # Clean up unused masters.
        self.cleanup_db(self.temp_base_db_file, self.temp_base_db_file)

        if os.path.exists(self.temp_base_db_file):
            os.rename(self.temp_base_db_file, self.deltaDebug_result_base_file)
        if os.path.exists(self.original_base_db_file):
            os.rename(self.original_base_db_file, self.base_db_file)

        print("___________________________________")
        print(f"Resultant file is {self.deltaDebug_result_base_file}")
        print("Delta Debugging Done!")


def main(args=None):
    parser = _build_parser()
    opt = parser.parse_args(args)
    whittler = Whittler(opt)
    whittler.debug()


if __name__ == "__main__":
    main()
