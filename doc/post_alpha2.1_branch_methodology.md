**<span style="text-decoration:underline;">Openroad project branching methodology post alpha2.1</span>**

**Motivation : **

Alpha2.1 has completed and we are back into a normal development cycle. With the new openroad app, some of the procedures we had put in place were poorly specified and a bit cumbersome. The goal of this document is to specify clearly an efficient branching methodology for the top level openroad app and also the submodules that it depends on. This methodology is efficient for developers and still allows us to maintain a stable externally facing branch.

**Procedure :**

The top level openroad app and all submodules which it depends on will develop in a branch called “openroad”. All repo’s included in the openroad app will use this branch name for consistency and so it’s easy to know which branch to checkout when working. The openroad app will use its git submodule hash to point to a stable commit of the submodules. This setup replaces our previous procedure of developing in a branch named “develop” and merging to master when stable. In many cases in the past this procedure was shortcut with code being submitted to develop and the submodule deriving its hash from a commit to develop. There is no reason to have this two step process in the submodules when we now have the top level app’s submodule hash. The submodule hash can ensure that no bad code is seen by the top level app and that it remains stable. The top level openroad app will also have a master branch which is externally facing and which remains very stable. We will run some combination of fast and slow regressions before merging to the top level app’s master, whereas in normal development on the openroad branch we will continue to commit and merge with fast regressions only.

