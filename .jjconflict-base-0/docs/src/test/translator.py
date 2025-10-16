import os
from md_roff_compat import man2_translate, man3_translate

# Test objective: if the translator functionality works.
res_dir = os.path.join(os.getcwd(), "results/docs")
os.makedirs(res_dir, exist_ok=True)
man2_translate("translator.md", res_dir)
man3_translate("translator.txt", res_dir)
