import sys
import re

def parse_ci_log(log_file_path):
    """
    Reads a CI text log and extracts errors and failed tests.
    """
    errors_found = []
    
    try:
        with open(log_file_path, 'r', encoding='utf-8') as file:
            for line in file:
                # Search for lines containing the word 'error'
                if re.search(r'\berror\b', line, re.IGNORECASE):
                    errors_found.append(line.strip())
                    
                # Search for lines indicating a failed test
                elif 'FAIL:' in line:
                    errors_found.append(line.strip())

    except FileNotFoundError:
        print(f"Error: Could not find log file at {log_file_path}")
        return
    
    # Format the output nicely using Markdown
    print("### 🚨 CI Failure Summary")
    if not errors_found:
        print("No explicit errors found in the log.")
    else:
        print(f"Found {len(errors_found)} critical issues:")
        for err in errors_found[:10]:  # Cap at 10 to avoid spamming the PR
            print(f"- `{err}`")

# This allows the script to be run directly from the command line
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 ci_summarizer.py <path_to_log_file>")
    else:
        parse_ci_log(sys.argv[1])