import os
import sys
from md_roff_compat import man2, man3

# Test objective: if the translator script can run without errors for all the docs.
# goal is to return an error if the header cannot be extracted.

# check man2
SRC_BASE_PATH = "../src"
DEST_BASE_PATH = "./md/man2"

# Create the destination directory if it doesn't exist
os.makedirs(DEST_BASE_PATH, exist_ok=True)

# Loop through all folders inside "../src"
for module_path in os.listdir(SRC_BASE_PATH):
    full_module_path = os.path.join(SRC_BASE_PATH, module_path)

    if os.path.isdir(full_module_path):
        module = os.path.basename(full_module_path)
        src_path = os.path.realpath(os.path.join(SRC_BASE_PATH, module, "README.md"))
        dest_path = os.path.realpath(os.path.join(DEST_BASE_PATH, module + ".md"))

        # Check if README.md exists before copying
        if os.path.exists(src_path):
            # Create a symbolic link from src_path to dest_path
            os.symlink(src_path, dest_path)
            print(f"File linked successfully.")
        else:
            print(f"ERROR: README.md not found in {full_module_path}")

# Run man2 command
man2()
