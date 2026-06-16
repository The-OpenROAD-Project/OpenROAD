import os

cur_dir = os.path.dirname(os.path.abspath(__file__))
messages_path = os.path.join(cur_dir, "../messages.txt")

with open(messages_path, encoding="utf-8") as messages_file:
    messages = messages_file.read()

expected_messages = [
    "GPL 0023",
    "Placement target density:",
    "GPL 0036",
    "Movable instances area:",
    "GPL 0084",
    "---- Execute Nesterov Global Placement.",
    "GPL 1002",
    "Placed Cell Area",
    "GPL 1003",
    "Available Free Area",
    "GPL 1017",
    "Routability mode iteration count:",
]

missing = [message for message in expected_messages if message not in messages]
assert not missing, f"messages.txt missing expected GPL entries: {missing}"
