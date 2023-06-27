import os

command = "python ../etc/find_messages.py -d ../src"
output = os.popen(command).read()

# Print the captured output
#print(output)

with open('user/MessagesFinal.md', 'w') as f:
    f.write("# OpenROAD Messages Glossary\n")
    f.write("Listed below are the OpenROAD warning/error codes you may encounter while using the application.\n")
    f.write("\n")
    f.write("| Tool | Code | Message                                             |\n")
    f.write("| ---- | ---- | --------------------------------------------------- |\n")

    lines = output.split('\n')
    for line in lines:
        columns = line.split()
        if not columns: continue
        ant = "[" + columns[0] + "](" + columns[-1] + ")"
        num = columns[1]
        message = " ".join(columns[3:-1])
        f.write(f"| {ant} | {num} | {message} |\n")
