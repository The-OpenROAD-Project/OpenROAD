import helpers
import os
import sys
import glob
sys.path.append('../scripts/')

from md_roff_compat import man2, man3
from md_roff_compat import docs2, docs3

# Test objective: if the translator script can run without errors for all the docs.
# goal is to return an error if the header cannot be extracted. 

# TODO: run link_readmes to get the files INSIDE docker.
##       thereafter remove the files by running `make clean`

# check man2 
os.chdir('../../')
man2()

# check man3
# for d in all_docs:
#     text = open(d).read()
#     tool_name, _ = os.path.splitext(os.path.basename(d))
#     func_file = helpers.make_result_file(f"{tool_name}.func")
#     write_funcs(funcs, func_file)

#     # write_funcs(funcs, f"{tool_name}.funcok")
    
#     output = helpers.diff_files(func_file, f"{tool_name}.funcok")