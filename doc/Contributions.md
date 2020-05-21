**OpenROAD Project Contribution Guide**

Thank you for taking the time to read this document and to contribute, the OpenROAD project will not reach all of its objectives without help!

Possible ways to contribute



*   Open Source PDK information 
*   Open Source Designs
*   Useful scripts
*   Tool improvements
*   New tools
*   Improving documentation including this document
*   Star our project and repos so we can see the number of people interested

**<span style="text-decoration:underline;">Licensing Contributions:</span>**

As much as possible, all contributions should be licensed using the BSD3 license. You can propose another license if you must but contributions made with BSD3 fit in the spirit of OpenROAD’s permissively open source philosophy.  We do have exceptions in the project but over time we hope that all contributions will be BSD3, or some other permissive license.

**<span style="text-decoration:underline;">Contributing Open Source PDK information and Designs:</span>**

If you have new design or PDK information to contribute, please add this to the repo [OpenROAD-flow](https://github.com/The-OpenROAD-Project/OpenROAD-flow/). In the flow [directory](https://github.com/The-OpenROAD-Project/OpenROAD-flow/tree/master/flow) you will see a directory for [designs](https://github.com/The-OpenROAD-Project/OpenROAD-flow/tree/master/flow/designs) with Makefiles to run them, and one for PDK [platforms](https://github.com/The-OpenROAD-Project/OpenROAD-flow/tree/master/flow/platforms/) used by the designs. If you add a new PDK platform be sure to add at least one design that uses it.

**<span style="text-decoration:underline;">Contributing Scripts and Code:</span>**

We follow the Google C++ style [guide](https://google.github.io/styleguide/cppguide.html)<span style="text-decoration:underline;">. </span>If you find code that is not following this guide, within each file that you edit, follow the style in that file. Please pay careful attention to the tool [checklist](https://github.com/The-OpenROAD-Project/OpenROAD/blob/e3fc17cdf2b49d7a946fe29780604a94c2146d14/doc/OpenRoadArch.md#tool-checklist) for all code. If you want to add or improve functionality in OpenROAD please start with the top level [app](https://github.com/The-OpenROAD-Project/OpenROAD/) repo. You can see in the src directory that submodules exist pointing to tested versions of the other relevant repos in the project. Please look at the tool workflow in the code architecture [document ](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/doc/OpenRoadArch.md)to work with the app and its submodule repos in an efficient way.

Please pay attention to the test [directory ](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/test)and be sure to add tests for any code changes that you make with open sourceable PDK and design information. We provide the nandgate45 PDK in the OpenROAD-flow repo to help with this. Pull requests with code changes are unlikely to be accepted without accompanying test cases. There are many [example](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/test/gcd_flow1.tcl) tests. Each repo has a test directory as well with tests you should run and add to if you modify something in one of the submodules.

If you want to add a new tool please look in the [src/tool](https://github.com/The-OpenROAD-Project/OpenROAD/tree/add_tool/src/tool) directory of the add_tool branch for an example of how to add one. 

For changes that claim to improve QoR or PPA, please run many tests and ensure that the improvement is not design specific. There are designs in the [OpenROAD-flow](https://github.com/The-OpenROAD-Project/OpenROAD-flow/) repo which can be used unless the improvement is technology specific.

Do not add runtime or build dependencies without serious thought. For a project like OpenROAD with many application sub components, the software architecture can quickly get out of control. Changes with lots of new dependencies which are not necessary are less likely to be integrated.

If you want to add TCL code to define a new tool command look at pdngen as an example of how to do so. Take a look at the cmake [file ](https://github.com/The-OpenROAD-Project/OpenROAD/blob/26437d70f094abf564317c25803fd93a80f6dcc0/src/CMakeLists.txt)which automatically sources the tcl code and the tcl [code](https://github.com/The-OpenROAD-Project/OpenROAD/blob/openroad/src/pdngen/src/PdnGen.tcl) itself.

**<span style="text-decoration:underline;">Questions:</span>**

You can file git issues to ask questions, file issues or you can contact us via email openroad at eng.ucsd.edu

