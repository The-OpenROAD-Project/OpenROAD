import os
from md_roff_compat import man2_translate, man3_translate

# Test objective: Check man2/man3 items parsed.

cur_dir = os.getcwd()
save_dir = os.path.join(cur_dir, "results/docs")
os.makedirs(save_dir, exist_ok=True)

readme_path = os.path.join(cur_dir, "../README.md")
messages_path = os.path.join(cur_dir, "../messages.txt")

man2_translate(readme_path, save_dir)
man3_translate(messages_path, save_dir)
