################################
# You need to provide the script with 3 main arguments
# --base_db_path <the relative path to the db file to perform the step on>
# --error_string <the output that indicates a target error has occurred>
# --step <Command used to perform a step on the base_db file>
# You also have 1 additional argument
# --persistence <a value in [1,6] indicating maximum granularity where maximum
# granularity = 2^persistence>
# --use_stdout <a flag to either enable or disable detecting the error string
# from stdout in addition to stderr>
# --dump_def <a flag to either enable or disable dumping def per each step,
# could be used if the step acts on a def file rather than an odb>

# EXAMPLE COMMAND:
# Assuming running in a directory with the following files in:
# deltaDebug.py base.odb step.sh
# openroad -python deltaDebug.py \
#  --base_db_path base.odb \
#  --error_string <any_possible_error> \
#  --step './step.sh' \
#  --persistence 5  --use_stdout --dump_def

# N.B: step.sh shall read base.odb (or base.def in case the flag dump_def = 1)
# and operate on it where the script manipulates base.odb between steps to
# reduce its size.
################################

import odb
from openroad import Design
import os
import sys
import signal
import subprocess
import argparse
import shutil
import select
import time
from math import ceil
import errno
import enum

persistence_range = [1, 2, 3, 4, 5, 6]
cut_multiple = range(1, 128)

parser = argparse.ArgumentParser("Arguments for delta debugging")
parser.add_argument(
    "--base_db_path", type=str, help="Path to the db file to perform the step on"
)
parser.add_argument(
    "--error_string",
    type=str,
    help="The output that indicates target error has occurred",
)
parser.add_argument(
    "--step", type=str, help="Command used to perform step on the input odb file"
)
parser.add_argument(
    "--timeout",
    type=int,
    default=None,
    help="Specify initial timeout in seconds, default is to measure it",
)
parser.add_argument(
    "--multiplier",
    type=int,
    default=cut_multiple[0],
    choices=cut_multiple,
    help="Multiply number of cuts with this number",
)
parser.add_argument(
    "--persistence",
    type=int,
    default=persistence_range[0],
    choices=persistence_range,
    help="Indicates maximum input fragmentation; "
    "fragments = 2^persistence; value in " + ", ".join(map(str, persistence_range)),
)
parser.add_argument(
    "--use_stdout",
    action="store_true",
    help="Enables reading the error string from standard output",
)
parser.add_argument(
    "--exit_early_on_error",
    action="store_true",
    help=(
        "Exit early on unrelated errors to speed things up, "
        + "but risks exiting on false negatives."
    ),
)
parser.add_argument(
    "--dump_def",
    action="store_true",
    help="Determines whether to dumb def at each step in addition to the odb",
)


class cutLevel(enum.Enum):
    Nets = 0
    Insts = 1


class deltaDebugger:
    def __init__(self, opt):
        if not os.path.exists(opt.base_db_path):
            raise FileNotFoundError(
                errno.ENOENT, os.strerror(errno.ENOENT), opt.base_db_path
            )

        base_db_directory = os.path.dirname(opt.base_db_path)
        base_db_name = os.path.basename(opt.base_db_path)
        self.base_db_file = opt.base_db_path

        self.error_string = opt.error_string
        self.use_stdout = opt.use_stdout
        self.exit_early_on_error = opt.exit_early_on_error
        self.step_count = 1

        # Setting persistence for the run
        self.persistence = opt.persistence

        self.multiplier = opt.multiplier
        self.timeout = opt.timeout

        # Temporary file names to hold the original base_db file across the run
        self.original_base_db_file = os.path.join(
            base_db_directory, f"deltaDebug_base_original_{base_db_name}"
        )

        # Temporary file used to hold current base_db to ensure its integrity across cuts
        self.temp_base_db_file = os.path.join(
            base_db_directory, f"deltaDebug_base_temp_{base_db_name}"
        )

        # The name of the result file after running deltaDebug
        self.deltaDebug_result_base_file = os.path.join(
            base_db_directory, f"deltaDebug_base_result_{base_db_name}"
        )

        # This determines whether design def shall be dumped or not
        self.dump_def = opt.dump_def
        if self.dump_def != 0:
            self.base_def_file = self.base_db_file[:-3] + "def"

        # A variable to hold the base_db
        self.base_db = None

        # Debugging level
        # cutLevel.Insts starts with inst then nets,
        # cutLevel.Nets cuts nets only.
        self.cut_level = cutLevel.Insts

        # step command
        self.step = opt.step

    def debug(self):
        # copy original base db file to avoid overwriting it
        print("Backing up original base file.")
        shutil.copy(self.base_db_file, self.original_base_db_file)

        # Rename the base db file to a temp name to keep it from
        # overwriting across the two steps cut
        os.rename(self.base_db_file, self.temp_base_db_file)

        if self.timeout is None:
            # timeout used to measure the time the original input takes
            # to reach an error to use as standard timeout for different
            # cuts.
            self.timeout = 1e6  # Timeout in seconds

            # Perform a step with no cuts to measure timeout
            print(
                "Performing a step with the original "
                + "input file to calculate timeout."
            )
            error, _ = self.perform_step()
            if error is None:
                print("No error found in the original input file.")
                sys.exit(1)

        for self.cut_level in (cutLevel.Insts, cutLevel.Nets):
            while True:
                err = None
                self.n = 2  # Initial Number of cuts

                cuts = None

                while self.n <= (2**self.persistence):
                    error_in_range = None
                    j = 0
                    while j == 0 or j < cuts:
                        current_err, cuts = self.perform_step(cut_index=j)
                        self.step_count += 1
                        if current_err is not None:
                            # Found the target error with the cut DB
                            #
                            # This is a suitable level of detail to look
                            # for more errors, complete this level of detail.
                            err = current_err
                            error_in_range = current_err
                            self.prepare_new_step()
                        j += 1

                    if error_in_range is None:
                        # Increase the granularity of the cut in case target
                        # error not found
                        self.n *= 2
                    elif self.n >= 8:
                        # Found errors, decrease granularity
                        self.n = int(self.n / 2)
                    else:
                        break

                if err is None or cuts == 0:
                    break

        # Delete unused master-cells from design
        self.remove_unused_masters()

        # Change deltaDebug resultant base_db
        # file name to a representative name
        if os.path.exists(self.temp_base_db_file):
            os.rename(self.temp_base_db_file, self.deltaDebug_result_base_file)

        # Restoring the original base_db file
        if os.path.exists(self.original_base_db_file):
            os.rename(self.original_base_db_file, self.base_db_file)

        print("___________________________________")
        print(f"Resultant file is {self.deltaDebug_result_base_file}")
        print("Delta Debugging Done!")

    # A function that do a cut in the db, writes the base db to disk
    # and calls the step function, then returns the stderr of the step.
    def perform_step(self, cut_index=-1):
        # read base db in memory
        self.base_db = Design.createDetachedDb()
        self.base_db = odb.read_db(self.base_db, self.temp_base_db_file)

        # Cut the block with the given step index.
        # if cut index of -1 is provided it means
        # that no cut will be made.
        if cut_index != -1:
            self.cut_block(index=cut_index)

        # Write DB
        odb.write_db(self.base_db, self.base_db_file)
        if self.dump_def != 0:
            print("Writing def file")
            odb.write_def(self.base_db.getChip().getBlock(), self.base_def_file)

        cuts = self.get_cuts() if cut_index != -1 else None

        # Destroy the DB in memory to avoid being out-of-memory when
        # the step code is running
        if self.base_db is not None:
            self.base_db.destroy(self.base_db)
            self.base_db = None

        # Perform step, and check the error code
        start_time = time.time()
        error_string = self.run_command(self.step)
        end_time = time.time()

        # Handling timeout so as not to run the code for time
        # that is more than the original buggy code or a
        # buggy cut.
        if error_string is not None:
            self.timeout = max(120, 1.2 * (end_time - start_time))
            print(f"Error Code found: {error_string}")

        return error_string, cuts

    def run_command(self, command):
        poll_obj = select.poll()
        if self.use_stdout == 0:
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
            return self.poll(process, poll_obj, start_time)
        finally:
            try:
                os.killpg(os.getpgid(process.pid), signal.SIGKILL)
            except ProcessLookupError:
                # This is an inevitable race condition, ignore
                pass

    def poll(self, process, poll_obj, start_time):
        output = ""
        # None for any error code other than self.error_string
        error_string = None
        while True:
            # polling on the output of the process with a timeout of 1 second
            # to avoid busywaiting
            if poll_obj.poll(1):
                if self.use_stdout == 0:
                    output = process.stderr.readline()
                else:
                    output = process.stdout.readline()

                if output.find(self.error_string) != -1:
                    # found the error code that we are searching for.
                    error_string = self.error_string
                    break
                elif self.exit_early_on_error and output.find("ERROR") != -1:
                    # Found different error (bad cut) so we can just
                    # terminate early and ignore this cut.
                    break

            curr_time = time.time()
            if (curr_time - start_time) > self.timeout:
                print(f"Step {self.step_count} timed out!", flush=True)
                break

            if process.poll() is not None:
                break

        return error_string

    # A function to rename a smaller db file that produces the target error
    # to the temporary name used to load a base db to perform further
    # cutting on it.
    def prepare_new_step(self):
        # Delete the old temporary db file
        if os.path.exists(self.temp_base_db_file):
            os.remove(self.temp_base_db_file)
        # Rename the new base db file to the temp name to keep it
        # from overwriting across the two steps cut
        if os.path.exists(self.base_db_file):
            os.rename(self.base_db_file, self.temp_base_db_file)

    def clear_dont_touch_inst(self, inst):
        inst.setDoNotTouch(False)
        for iterm in inst.getITerms():
            net = iterm.getNet()
            if net:
                net.setDoNotTouch(False)

    def clear_dont_touch_net(self, net):
        net.setDoNotTouch(False)
        for iterm in net.getITerms():
            iterm.getInst().setDoNotTouch(False)

    def get_insts(self):
        return self.base_db.getChip().getBlock().getInsts()

    def get_nets(self):
        return self.base_db.getChip().getBlock().getNets()

    def get_elms(self):
        if self.cut_level == cutLevel.Insts:
            return self.get_insts()
        return self.get_nets()

    def get_cuts(self):
        return min(self.n * self.multiplier, len(self.get_elms()))

    # A function that cuts the block according to the given direction
    # and ratio. It also uses the class cut level  to identify
    # whether to cut Insts or Nets.
    def cut_block(self, index=0):
        message = [f"Step {self.step_count}"]
        if self.cut_level == cutLevel.Insts:  # Insts cut level
            elms = self.get_insts()
            message += ["Insts level debugging"]
        elif self.cut_level == cutLevel.Nets:  # Nets cut level
            elms = self.get_nets()
            message += ["Nets level debugging"]

        message += [f"Insts {len(self.get_insts())}", f"Nets {len(self.get_nets())}"]

        num_elms = len(elms)
        assert num_elms > 0

        cuts = self.get_cuts()
        start = num_elms * index // cuts
        end = num_elms * (index + 1) // cuts

        cut_position_string = "#" * cuts
        cut_position_string = (
            cut_position_string[:index] + "C" + cut_position_string[index + 1 :]
        )
        message += [f"cut elements {end-start}"]
        message += [f"timeout {ceil(self.timeout/60.0)} minutes"]
        self.cut_elements(start, end)

        cuts = self.n * self.multiplier
        print(", ".join(message), flush=True)
        print(f"[{cut_position_string}]", flush=True)

    def cut_elements(self, start, end):
        block = self.base_db.getChip().getBlock()
        if self.cut_level == cutLevel.Insts:  # Insts cut level
            elms = block.getInsts()
        elif self.cut_level == cutLevel.Nets:  # Nets cut level
            elms = block.getNets()

        for i in range(start, end):
            elm = elms[i]
            if self.cut_level == cutLevel.Insts:
                self.clear_dont_touch_inst(elm)
            elif self.cut_level == cutLevel.Nets:
                self.clear_dont_touch_net(elm)
            elm.destroy(elm)

    def remove_unused_masters(self):
        # Create new clean dbDatabase
        self.base_db = Design.createDetachedDb()
        print(f"Reading {self.temp_base_db_file}  file ")
        self.base_db = odb.read_db(self.base_db, self.temp_base_db_file)
        print("Removing unused masters...")
        unused = self.base_db.removeUnusedMasters()
        print(f"Removed {unused} masters.")
        odb.write_db(self.base_db, self.temp_base_db_file)

        if self.dump_def != 0:
            print("Writing def file")
            odb.write_def(
                self.base_db.getChip().getBlock(), self.temp_base_db_file[:-3] + "def"
            )

        if self.base_db is not None:
            self.base_db.destroy(self.base_db)
            self.base_db = None


if __name__ == "__main__":
    opt = parser.parse_args()
    debugger = deltaDebugger(opt)
    debugger.debug()
