import os

# Ensure path in OR root folder.
current_path = os.path.abspath(__file__)
os.chdir(
    os.path.dirname(os.path.dirname(os.path.dirname((os.path.dirname(current_path)))))
)

# First run for OR local files.
print("Writing OR local messages.")
os.system("python3 ./etc/find_messages.py -d ./src -l > ./messages.txt")

# List folders in ./src
for tool in os.listdir("./src"):
    if os.path.isdir(os.path.join("./src", tool)):
        if tool == "sta":
            continue
        print(f"Writing {tool} local messages.")
        os.system(
            f"python3 ./etc/find_messages.py -d ./src/{tool} > src/{tool}/messages.txt"
        )
