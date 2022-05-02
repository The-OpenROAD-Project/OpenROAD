# CI Guide

This document describes the pipelines available to the developers and code
maintainers in the [Jenkins server](https://jenkins.openroad.tools/). Note
that pipelines with the suffix `*-Private` are only available to code
maintainers and The OpenROAD Project members as they can contain confidential
information. Thus, to access Private pipelines one needs to have authorization
to access confidential data and be logged in the Jenkins website.

Below there is a list of the available features. Instructions on how to
navigate Jenkins to access these features are available
[here](https://docs.google.com/presentation/d/1kWHLjUBFcd0stnDaPNi_pt9WFrrsR7tQ95BGhT1yOvw/edit?usp=sharing).

-   Find your build through Jenkins website or from GitHub.
-   See test status: Pass/Fail.
-   Log files for each test.
-   Build artifacts to reproduce failures.
-   HTML reports about code coverage and metrics.

## OpenROAD App

-   OpenROAD-Coverage-Public
    -   Description: run dynamic code coverage tool `lconv`.
    -   Target: master branch.
    -   Report link [here](https://jenkins.openroad.tools/job/OpenROAD-Coverage-Public/Dynamic_20Code_20Coverage/).
-   OpenROAD-Coverity-Public
    -   Description: compile and submit builds to Coverity static code analysis
        tool.
    -   Target: master branch.
    -   Report link [here](https://scan.coverity.com/projects/the-openroad-project-openroad).
-   OpenROAD-Nightly-Public
    -   Description: `openroad` unit tests, docker builds, ISPD 2018 and 2019
        benchmarks for DRT and large unit tests of GPL.
    -   Target: master branch.
-   OpenROAD-Public
    -   Description: `openroad` unit tests and docker builds.
    -   Target: all branches and open PRs.
-   OpenROAD-Special-Private
    -   Description: for developer testing, runs ISPD 2018 and 2019 benchmarks
        for DRT and large unit tests of GPL.
    -   Target branches: `TR_*`, `secure-TR_*`, `TR-*`, `secure-TR-*`.
-   OpenROAD-Private
    -   Description: `openroad` unit tests and docker builds.
    -   Target: all branches. Note that PRs will be run on public side after
        "Ready to Sync Public" workflow.


## OpenROAD Flow

-   OpenROAD-flow-script-Public [folder]
    -   `public_tests_all`
        -   Description: runs flow tests except RTLMP designs. Should finish in
            less than three hours.
        -   Target: master branch.
    -   `public_tests_all-pr`
        -   Description: runs flow tests except RTLMP designs. Should finish in
            less than three hours.
        -   Target: all open PRs.
    -   `publish-results-to-dashboard`
        -   Description: uploads metrics to dashboard website.
        -   Target: master branch.
-   OpenROAD-flow-scripts-Nightly-Public
    -   Description: runs all flow tests including RTLMP designs.
    -   Target: master branch.
-   OpenROAD-flow-scripts-Private [folder]
    -   `public_tests_small`
        -   Description: runs fast flow tests, does not include RTLMP designs.
            Should finish in less than one hour.
        -   Target: all branches that are not filed as PR. CI will run for PR
            branches on the public side after "Ready to Sync Public" workflow.
-   OpenROAD-flow-scripts-All-Tests-Private
    -   Description: runs flow tests, does not include RTLMP designs.
    -   Target: secure branches.


## OpenLane

-   OpenLane-MPW-CI-Public
    -   Description: test projects to older MPW shuttles with newer OpenLane versions.
    -   [Repo link](https://github.com/The-OpenROAD-Project/OpenLane-MPW-CI).
-   OpenLane-Public
    -   Description: test OpenLane with latest commit from OpenROAD.
    -   [Repo link](https://github.com/The-OpenROAD-Project/OpenLane).
