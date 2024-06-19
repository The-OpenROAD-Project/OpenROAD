import os
import subprocess
import re

def run_regression_script(regression_script_path, argument):
    try:
        subprocess.run([regression_script_path, argument], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running regression script: {e}")
        return False
    return True

def check_log_file(log_path):
    pattern = re.compile(r"repair_design: runtime.*seconds.*usage.*rsz.*MB.*vsz.*MB.*peak.*rsz.*vsz.*")
    runtime_count = 0

    with open(log_path, 'r') as log_file:
        for line in log_file:
            if pattern.search(line):
                runtime_count += 1
    if runtime_count == 2:
        print("Passed: Statistics are logged.")
        exit(0)
    else:
        print("Statistics are not logged enough times.")
        exit(1)

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    base_dir = os.path.abspath(os.path.join(script_dir, "../../rsz/test"))
    log_path = os.path.join(base_dir, "results", "repair_design4-tcl.log")
    regression_script_path = os.path.join(base_dir, "regression")

    if not os.path.exists(log_path):
        if not run_regression_script(regression_script_path, "repair_design4"):
            exit(1)

    check_log_file(log_path)

if __name__ == "__main__":
    main()

