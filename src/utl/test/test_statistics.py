import os
import re
from openroad import Tech, Design

def check_log_file(log_path):
    pattern = re.compile(r"global.*route.*cpu.*time.*elapsed.*time.*memory.*peak.*")
    match_count = 0

    with open(log_path, 'r') as log_file:
        for line in log_file:
            if pattern.search(line):
                match_count += 1

    return match_count == 1

def main():
    test_path = os.path.abspath(os.path.dirname(__file__))

    tech = Tech()
    tech.readLef(os.path.join(test_path, "../../../test/Nangate45/Nangate45.lef"))

    design = Design(tech)
    gcddef = os.path.abspath(os.path.join(test_path, "../../grt/test/gcd.def"))
    design.readDef(gcddef)

    gr = design.getGlobalRouter()
    gr.globalRoute(True)

    log_path = os.path.join(test_path, "results", "test_statistics-py.log")
    if check_log_file(log_path):
        with open(log_path, 'w') as log_file:
            log_file.write("Passed: Statistics are logged.\n")
        exit(0)
    else:
        with open(log_path, 'w') as log_file:
            log_file.write("Failed: Statistics are not logged.\n")
        exit(1)

if __name__ == "__main__":
    main()

