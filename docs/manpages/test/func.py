import helpers
import os
import sys
import glob
sys.path.append('../scripts/')

from md_roff_compat import extract_headers

# to repeat this test for ALL original docs (before preprocessing). 
# Function goal: to extract headers of the form (# HEADER 1) and convert 
# goal is to return an error if the header cannot be extracted. 

def write_funcs(funcs, fname):
    with open(fname, "w") as f:
        f.write("\n".join(funcs))

all_docs = glob.glob("../src/*/*.txt")

for d in all_docs:
    text = open(d).read()
    tool_name, _ = os.path.splitext(os.path.basename(d))
    func_file = helpers.make_result_file(f"{tool_name}.func")
    funcs = extract_headers(text, 3)
    write_funcs(funcs, func_file)

    # write_funcs(funcs, f"{tool_name}.funcok")
    
    output = helpers.diff_files(func_file, f"{tool_name}.funcok")