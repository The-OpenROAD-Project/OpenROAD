import os
import sys
sys.path.append('../scripts/')

from md_roff_compat import man2, man3

# Test objective: if the translator script can run without errors for all the docs.
# goal is to return an error if the header cannot be extracted. 

# check man2 
os.chdir('../../')
os.system("./man/scripts/link_readmes.sh")
man2()

# check man3
man3()
os.system("make clean -s")