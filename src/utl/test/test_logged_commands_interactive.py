import os
import subprocess
import sys

# Finding out what commands were called
def log_called_commands(input_code, log_file):
    with open(log_file, 'w') as log:
        lines = input_code.strip().split('\n')
        for line in lines:
            log.write(line + "\n")
            if line.startswith("source "):
                source_file = line.strip().split(" ")[1]
                if os.path.isfile(source_file):
                    with open(source_file, 'r') as sourced_file:
                        log.write(sourced_file.read())
                    log.write("\n")
                else:
                    print(f"Error: Sourced file '{source_file}' not found.")
                    sys.exit(1)

# Executing the openroad
def execute_openroad(commands, log_file, openroad_path):
    result = subprocess.run(
        [openroad_path],
        input=commands,
        text=True,
        capture_output=True
    )
    with open(log_file, 'w') as f:
        f.write(result.stdout)
        f.write(result.stderr)
    return result.stdout, result.stderr

# Checking if the called commands are logged in the log file
def compare(commands, log_file, openroad_path):
    expected_log = "expected_commands.tcl"

    stdout, stderr = execute_openroad(commands, log_file, openroad_path)
    log_called_commands(commands, expected_log)

    with open(expected_log, 'r') as expected:
        expected_lines = [line.strip() for line in expected if line.strip()]

    log_content = stdout.splitlines() + stderr.splitlines()

    for log_line in log_content:
        if log_line.startswith("invalid command name"):
            invalid_command = log_line.split('"')[1]
            print(f"Warning: Command '{invalid_command}' is not a valid command name.")

    for code_line in expected_lines:
        if not any(code_line in log_line for log_line in log_content):
            print(f"Error: Command '{code_line}' is not logged in '{log_file}'.")
            
if __name__ == "__main__":
    log_file = "file.log"
    cur_dir = os.path.dirname(os.path.realpath(__file__))
    for _ in range(5):
        cur_dir = os.path.dirname(cur_dir)
    cur_dir = os.path.join(cur_dir, 'tools/install/OpenROAD/bin/openroad')
    openroad_path = cur_dir
    commands = """puts "hello world"
user_run_time
unset_timing_derate
show_splash
exit"""

    compare(commands, log_file, openroad_path)