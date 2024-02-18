# Git Quickstart

This tutorial serves as a quickstart to Git and contributing to our repository. If you have not already set up OpenROAD, please follow the instructions [here](../user/Build.md). 

```{tip} This basic tutorial gives instruction for basic password Git authentication.
If you would like to setup SSH authentication, please follow this [guide](https://help.github.com/set-up-git-redirect).
```

## Forking

You will need your own fork to work on the code. Go to the `OpenROAD` project
[page](https://github.com/The-OpenROAD-Project/OpenROAD) and hit the `Fork` button. You will
want to clone your fork to your machine:

```shell
git clone https://github.com/your-user-name/OpenROAD.git
cd OpenROAD
git remote add upstream https://github.com/The-OpenROAD-Project/OpenROAD.git
git fetch upstream
```

This creates the directory `OpenROAD` and connects your repository to
the upstream (master project) *OpenROAD* repository.

## Creating a branch

You want your master branch to reflect only production-ready code, so create a
feature branch for making your changes. For example:

```shell
git checkout master && git branch shiny-new-feature
git checkout shiny-new-feature
# Or equivalently, 
git checkout master && checkout -b shiny-new-feature 
```

This changes your working directory to the shiny-new-feature branch.  Keep any
changes in this branch specific to one bug or feature so it is clear
what the branch brings to OpenROAD. You can have many shiny-new-features
and switch in between them using the git checkout command.

When creating this branch, make sure your master branch is up to date with
the latest upstream master version. To update your local master branch, you
can do:

```shell
git checkout master
git pull upstream master
```

When you want to update the feature branch with changes in master after
you created the branch, check the section on 
[updating a PR](#updating-your-pull-request).

## Committing your code
Keep style fixes to a separate commit to make your pull request more readable. Once you've made changes, you can see them by typing:

```shell
git status
```

If you have created a new file, it is not being tracked by git. Add it by typing:
```shell
git add path/to/file-to-be-added.py
```

Doing `git status` again should give something like:
```shell
# On branch shiny-new-feature
#
#       modified:   /relative/path/to/file-you-added.py
#
```

Finally, commit your changes to your local repository with an explanatory commit
message. Do note the `-s` option is needed for developer signoff. 
```shell
git commit -s -m "your commit message goes here"
```

## Pushing your changes

When you want your changes to appear publicly on your GitHub page, push your
forked feature branch's commits:

```shell
git push origin shiny-new-feature
```

Here `origin` is the default name given to your remote repository on GitHub.
You can see the remote repositories:

```shell
git remote -v
```

If you added the upstream repository as described above you will see something
like:

```shell
origin  https://github.com/your-user-name/OpenROAD.git (fetch)
origin  https://github.com/your-user-name/OpenROAD.git (push)
upstream        https://github.com/The-OpenROAD-Project/OpenROAD.git (fetch)
upstream        https://github.com/The-OpenROAD-Project/OpenROAD.git (push)
```

Now your code is on GitHub, but it is not yet a part of the OpenROAD project. For that to
happen, a pull request needs to be submitted on GitHub.

## Review your code

When you're ready to ask for a code review, file a pull request. Before you do, once
again make sure that you have followed all the guidelines outlined in the [Developer's Guide](./DeveloperGuide.md)
regarding code style, tests, performance tests, and documentation. You should also
double check your branch changes against the branch it was based on:

1. Navigate to your repository on GitHub -- https://github.com/your-user-name/OpenROAD
1. Click on `Branches`
1. Click on the `Compare` button for your feature branch
1. Select the `base` and `compare` branches, if necessary. This will be `master` and
   `shiny-new-feature`, respectively.

## Submitting the pull request

If everything looks good, you are ready to make a pull request. A pull request is how
code from a local repository becomes available to the GitHub community and can be looked
at and eventually merged into the master version. This pull request and its associated
changes will eventually be committed to the master branch and available in the next
release. To submit a pull request:

1. Navigate to your repository on GitHub
1. Click on the ``Compare & pull request`` button
1. You can then click on ``Commits`` and ``Files Changed`` to make sure everything looks
   okay one last time
1. Write a description of your changes in the ``Preview Discussion`` tab
1. Click ``Send Pull Request``.

This request then goes to the repository maintainers, and they will review
the code.

## Updating your pull request

Based on the review you get on your pull request, you will probably need to make
some changes to the code. In that case, you can make them in your branch,
add a new commit to that branch, push it to GitHub, and the pull request will be
automatically updated.  Pushing them to GitHub again is done by:

```shell
git push origin shiny-new-feature
```

This will automatically update your pull request with the latest code and restart the
[Continuous Integration](./CI.md) tests.

Another reason you might need to update your pull request is to solve conflicts
with changes that have been merged into the master branch since you opened your
pull request.

To do this, you need to `merge upstream master` in your branch:

```shell
git checkout shiny-new-feature
git fetch upstream
git merge upstream/master
```

If there are no conflicts (or they could be fixed automatically), a file with a
default commit message will open, and you can simply save and quit this file.

If there are merge conflicts, you need to solve those conflicts. See 
this [article](https://help.github.com/articles/resolving-a-merge-conflict-using-the-command-line/)
for an explanation on how to do this.
Once the conflicts are merged and the files where the conflicts were solved are
added, you can run ``git commit`` to save those fixes.

If you have uncommitted changes at the moment you want to update the branch with
master, you will need to ``stash`` them prior to updating. 

```{seealso}
See the stash [docs](https://git-scm.com/book/en/v2/Git-Tools-Stashing-and-Cleaning).
```
This will effectively store your changes and they can be reapplied after updating.

After the feature branch has been updated locally, you can now update your pull
request by pushing to the branch on GitHub:

```shell
git push origin shiny-new-feature
```

## Tips for a successful pull request

If you have made it to the `Review your code` phase, one of the core contributors may
take a look. Please note however that a handful of people are responsible for reviewing
all of the contributions, which can often lead to bottlenecks.

To improve the chances of your pull request being reviewed, you should:

- **Reference an open issue** for non-trivial changes to clarify the PR's purpose
- **Ensure you have appropriate tests**. These should be the first part of any PR
- **Keep your pull requests as simple as possible**. Larger PRs take longer to review
- **Ensure that CI is in a green state**. Reviewers may not even look otherwise
- **Keep updating your pull request**, either by request or every few days

## Acknowledgements

This page has been adapted from [pandas Developer Guide](https://pandas.pydata.org/docs/development/contributing.html). 
