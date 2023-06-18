#!/bin/bash

python ../etc/find_messages.py -d ../src > user/Messages.md

input_file="user/Messages.md"
output_file="user/MessagesFinal.md"

# Create file header
echo "# OpenROAD Messages Glossary" > "$output_file"
echo "Listed below are the OpenROAD warning/error codes you may encounter during running." >> "$output_file"
echo "" >> "$output_file"

# Create the Markdown table header
echo "| Tool | Code | Message                                             |" >> "$output_file"
echo "| ---- | ---- | --------------------------------------------------- |" >> "$output_file"

awk '{
  ant=$1;
  num=$2;
  message=substr($0, index($0,$4));
  printf("| %s | %s | %s |\n", ant, num, message);
}' "$input_file" >> "$output_file"

