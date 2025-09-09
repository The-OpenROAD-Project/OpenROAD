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

-  Information about OpenROAD Flow CI jobs can be found [here](https://openroad-flow-scripts.readthedocs.io/en/latest/contrib/CI.html)

## OpenLane

-   OpenLane-MPW-CI-Public
    -   Description: test projects to older MPW shuttles with newer OpenLane versions.
    -   [Repo link](https://github.com/The-OpenROAD-Project/OpenLane-MPW-CI).
-   OpenLane-Public
    -   Description: test OpenLane with latest commit from OpenROAD.
    -   [Repo link](https://github.com/The-OpenROAD-Project/OpenLane).
