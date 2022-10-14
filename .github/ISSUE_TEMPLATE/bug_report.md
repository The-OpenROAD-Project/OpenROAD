---
name: Bug report
about: Create a report to help us improve
title: ''
labels: ''
assignees: ''

---

<!--
    This issue template is REQUIRED for all bug reports. It helps us more
    quickly track, narrow down, and address bugs. 

    Bug reports not adhering to this template will be likely marked invalid and
    closed as there is very little we can do without the requisite information.

    Thank you for understanding!
-->

**Describe the bug**
<!-- A clear and concise description of what the bug is.  -->

**Expected Behavior**
<!-- A clear and concise description of what you expected to happen. -->

**Environment**
<!--
    This part is incredibly important:

    Please run the following shell command in the OpenROAD root folder:

        ./etc/Env.sh

    If you are an OpenLane user, please run the following shell command in the OpenLane root folder:

        python3 ./env.py issue-survey

    Then copy and paste the ENTIRE output between the triple-backticks below. (```)

    If the command does not succeed, you are using an out-of-date version of
    OpenROAD, and it is recommended that you update.
-->
```
YOUR SURVEY HERE
```

**To Reproduce**
<!--
    If you are running from OpenLane, please run the following shell command in the OpenLane root folder:
            python3 ./scripts/or_issue.py\
                --tool openroad\
                --script ./scripts/openroad/<script-name>\
                <run-path> # e.g designs/spm/runs/RUN_2022.03.01_19.21.10/tmp/routing/17-fill.def
                # run path is implicitly specified by input def

    For more information, follow this link https://openlane.readthedocs.io/en/latest/for_developers/using_or_issue.html

    Otherwise, you have two options here:

    A. Use `make <SCRIPT_NAME>_issue` to create a tar file with all the files to reproduce the bug(s).
        Steps:
        1. Clone/Use [OpenROAD-flow-scripts](https://github.com/The-OpenROAD-Project/OpenROAD- 
            flow-scripts.git) and make sure it is using the corresponding version of OpenROAD in path 
             "OpenROAD-flow-scripts/tools/OpenROAD"
        2. Set the ISSUE_TAG variable to rename the generated tar file
        3. Run the following shell command in this directory "OpenROAD-flow-scripts/flow"
                                        `make {script}_issue`
             where script is wildcarded from the "OpenROAD-flow-scripts/scripts" directory 
                                        # e.g "make cts_issue"
        4. Upload the generated tar file

    B. Upload relevant files
        * Upload a tar file containing the relevant files (.def, .lef and flow.tcl).
        * List the commands used.
-->


**Logs**
<!--
    Feel free to add any relevant log snippets to this section.
    
    Please do ensure they're inside the triple-backticks. (```)
-->
```
YOUR LOGS HERE
```

**Screenshots**
<!-- If applicable, add screenshots to help explain your problem.-->

**Additional context**
<!--  Add any other context about the problem here. -->
