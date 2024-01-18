import re

def extract_headers(text, level = 1):
    assert isinstance(level, int) and level >= 1
    pattern = r'^#{%d}\s+(.*)$' % level
    headers = re.findall(pattern, text, flags=re.MULTILINE)
    # TODO: Handle developer commands
    #if "Useful Developer Commands" in headers: headers.remove("Useful Developer Commands")
    return headers

def extract_description(text):
    # this is so that it always tries to match the longer headers first, to disambiguate
    sorted_headers = sorted(extract_headers(text,3), key=len, reverse=True)
    headers = "|".join(re.escape(x) for x in sorted_headers)
    pattern = rf'### ({headers})(.*?)```tcl'
    custom_strings = re.findall(pattern, text, flags=re.DOTALL)
    return [custom_string[1].strip() for custom_string in custom_strings]

def extract_tcl_code(text):
    pattern = r'```tcl\s+(.*?)```'
    tcl_code_matches = re.findall(pattern, text, flags=re.DOTALL)
    # remove the last tcl match
    tcl_code_matches = [x for x in tcl_code_matches if "./test/gcd.tcl" not in x]
    return tcl_code_matches

def extract_arguments(text):
    # Goal is to extract all the text from the end of tcl code to the next ### header.
    # Returns options and arguments.
    level2 = extract_headers(text, 2)
    level3 = extract_headers(text, 3)

    # form these 2 regex styles. 
    # ### Header 1 {text} ### Header2; ### Header n-2 {text} ### Header n-1
    # ### Header n {text} ## closest_level2_header
    first = [rf'### ({level3[i]})(.*?)### ({level3[i+1]})'  for i in range(len(level3) - 1)]

    # find the next closest level2 header to the last level3 header.
    closest_level2 = [text.find(f"## {x}") - text.find(f"### {level3[-1]}") for x in level2]
    closest_level2_idx = [idx for idx, x in enumerate(closest_level2) if x > 0][0]

    # This will disambiguate cases where different level headers share the same name.
    second = [rf"### ({level3[-1]})(.*?)## ({level2[closest_level2_idx]})"]
    final_options, final_args = [], []
    for idx, regex in enumerate(first + second):
        match = re.findall(regex, text, flags = re.DOTALL)
        #print(regex)
        # get text until the next header
        a = match[0][1] 
        a = a[a.find("#"):]
        options = a.split("####")[1:]
        if not options:
            final_options.append([])
            final_args.append([])
            continue
        options, args = options[0], options[1:]
        final_options.append(extract_tables(options))
        tmp_arg = []
        for arg in args:
            tmp_arg.extend(extract_tables(arg))
        final_args.append(tmp_arg)
    return final_options, final_args

def extract_tables(text):
    # Find all lines that start with "|"
    table_pattern = r'^\s*\|.*$'
    table_matches = re.findall(table_pattern, text, flags=re.MULTILINE)

    # Exclude matches containing HTML tags
    table_matches = [table for table in table_matches if not re.search(r'<.*?>', table)]

    # Remove text containing switch 
    table_matches = [table for table in table_matches if "Switch Name" not in table]

    # Remove text containing "---"
    table_matches = [table for table in table_matches if "---" not in table]

    return table_matches

def parse_switch(text):
    # Find the index of the 1nd and last occurrence of "|". Since some content might contain "|"
    switch_name = text.split("|")[1]
    switch_name = switch_name.replace("`", "").strip()
    second_pipe_index = text.find("|", text.find("|") + 1)
    last_pipe_index = text.rfind("|")
    switch_description = text[second_pipe_index+1: last_pipe_index-1] 
    return switch_name, switch_description


if __name__ == "__main__":
    pass