#!/usr/bin/env python3

import os
import yaml

dir_path = os.path.dirname(os.path.realpath(__file__))

yaml_path = os.path.join(dir_path, "variables.yaml")
with open(yaml_path, "r") as file:
    data = yaml.safe_load(file)

for key, value in data.items():
    if value.get("default", None) is None:
        continue
    print(f'export {key}?={str(value["default"]).replace(" ", "__SPACE__")}')
