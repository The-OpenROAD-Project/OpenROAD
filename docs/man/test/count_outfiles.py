import os
import sys
sys.path.append('../scripts/')

# Test objective: To count the number of expected files in man2/man3

# This script mimics the build process and checks expected output.
os.chdir("../../")
os.system("make clean -s && make preprocess -s")
os.system("make all -j16 -s")

# This tracks how many modules are being generated for man2.
# Please change this if modules are added!
MAN2_COUNT = 24


# Check if the files in html1 == cat1, html2 == cat2, html3 = cat3
for i in range(1, 4):
    man_dir = f"man/man{i}"
    html_dir = f"html/html{i}"
    cat_dir = f"cat/cat{i}"
    assert os.path.exists(man_dir), f"{man_dir} does not exist."
    assert os.path.exists(html_dir), f"{html_dir} does not exist."
    assert os.path.exists(cat_dir), f"{cat_dir} does not exist."
    man_count = len(os.listdir(man_dir))
    html_count = len(os.listdir(html_dir))
    cat_count = len(os.listdir(cat_dir))

    if man_count == html_count == cat_count:
        print(f"Level {i} doc counts are equal to {html_count}.")
    else:
        print(f"Level {i} doc counts are not equal.")
        print(f"{html_dir} count: {html_count}\n\
                {cat_dir} count: {cat_count}\n\
                {man_dir} count: {man_count}")