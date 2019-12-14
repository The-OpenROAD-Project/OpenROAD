**<span style="text-decoration:underline;">Philosophy :</span>**

As of 12/13/2019, besides Tapcell and Macroplace, we don't need any further integration into the top level app based on where we are today. We have done enough to prove that we can do the rest far before July and that we have a nicely architected app. We can use makefile targets and db file handoffs cleanly for non integrated steps in the flow. We can accept more integration changes but not by default. The integration causes churn as we have seen. We have to focus on making what we have work now. Once we have the 4 design columns green and openroad-flow set up with CI then we can accept more integration work but not until then. We need to converge the release. 

**<span style="text-decoration:underline;">Mechanics :</span>**

Starting tomorrow at 5pm Pacific Time on Friday 12/13



*   For the submodules, the developers of each repo should make a branch named “alpha2” and populate it from their openroad branch. The openroad app will also create this branch. The “alpha2” branch should not to be confused with alpha1 last July.
*   Do not merge or push to the alpha2 branch without permission but keep your active code in other branches.
*   Permission to modify the alpha2 branches can be given by Cherry, Tom or Austin with Tom or Cherry needed to approval the actual pull request diffs.
*   For continuing work not for alpha2, each repo internal to OpenROAD should have a branch named "openroad" which the git submodules will get their git hashes from. At the moment the branch naming for the submodule repos is random. We need to unify this.
*   For developers not working on January critical issues, they should not slow down but rather continuing working in branches as usual until we are ready for their changes.
*   Documentation can be updated at any time, please update your README’s
*   The OpenROAD-flow repo will be changing as we get more designs to work. The scripts there will be have control added when get to the demo freeze on 12/20. At that time we should also make an alpha_release_2.0.0 branch there.
*   Note : We will not support branches like alpha2 for the long term but use them for their purpose, like the integration exercise, and then move on as we did with alpha1 in July.

<!-- Docs to Markdown version 1.0β17 -->
