#!/usr/bin/env python3
import re
import sys
import os
import argparse  # argument parsing

# WARNING: this script expects the tech lef first

# Parse and validate arguments
# ==============================================================================
parser = argparse.ArgumentParser(
    description='Merges lefs together')
parser.add_argument('--inputLef', '-i', required=True,
                    help='Input Lef', nargs='+')
parser.add_argument('--outputLef', '-o', required=True,
                    help='Output Lef')
args = parser.parse_args()


print(os.path.basename(__file__),": Merging LEFs")

f = open(args.inputLef[0])
content = f.read()
f.close()


# Using a set so we get unique entries
propDefinitions = set()

# Remove Last line ending the library
content = re.sub("END LIBRARY","",content)

# Iterate through additional lefs
for lefFile in args.inputLef[1:]:
  f = open(lefFile)
  snippet = f.read()
  f.close()

  # Match the sites
  pattern = r"(^SITE (\S+).*?END\s\2)"
  m = re.findall(pattern, snippet, re.M | re.DOTALL)

  print(os.path.basename(lefFile) + ": SITEs matched found: " + str(len(m)))
  for groups in m:
    content += "\n" + groups[0]


  # Match the macros
  pattern = r"(^MACRO (\S+).*?END\s\2)"
  m = re.findall(pattern, snippet, re.M | re.DOTALL)

  print(os.path.basename(lefFile) + ": MACROs matched found: " + str(len(m)))
  for groups in m:
    content += "\n" + groups[0]

  # Match the property definitions
  pattern = r"^(PROPERTYDEFINITIONS)(.*?)(END PROPERTYDEFINITIONS)"
  m = re.search(pattern, snippet, re.M | re.DOTALL)

  if m:
    print(os.path.basename(lefFile) + ": PROPERTYDEFINITIONS found")
    propDefinitions.update(map(str.strip,m.group(2).split("\n")))


# Add Last line ending the library
content += "\nEND LIBRARY"

# Update property definitions

# Find property definitions in base lef
pattern = r"^(PROPERTYDEFINITIONS)(.*?)(END PROPERTYDEFINITIONS)"
m = re.search(pattern, content, re.M | re.DOTALL)
if m:
  print(os.path.basename(lefFile) + ": PROPERTYDEFINITIONS found in base lef")
  propDefinitions.update(map(str.strip,m.group(2).split("\n")))


replace = r"\1" + "\n".join(propDefinitions) + r"\n\3"
content = re.sub(pattern, replace, content, 0, re.M | re.DOTALL)

# Save the merged lef
f = open(args.outputLef, "w")
f.write(content)
f.close()

print(os.path.basename(__file__),": Merging LEFs complete")
