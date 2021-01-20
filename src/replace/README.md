# RePlAce
RePlAce: Advancing Solution Quality and Routability Validation in Global Placement

## Features
- Analytic and nonlinear placement algorithm. Solves electrostatic force equations using Nesterov's method. ([link](https://cseweb.ucsd.edu/~jlu/papers/eplace-todaes14/paper.pdf))
- Verified with various commercial technologies using OpenDB (7/14/16/28/45/55/65nm).
- Verified deterministic solution generation with various compilers and OS. 
  * Compiler: gcc4.8-9.1/clang-7-9/apple-clang-11
  * OS: Ubuntu 16.04-18.04 / CentOS 6-8 / OSX 
- Cleanly rewritten as C++11.
- Supports Mixed-size placement mode.
- Supports fast image drawing modes with CImg library.

| <img src="./doc/image/adaptec2.inf.gif" width=350px> | <img src="./doc/image/coyote_movie.gif" width=400px> | 
|:--:|:--:|
| *Visualized examples from ISPD 2006 contest; adaptec2.inf* |*Real-world Design: Coyote (TSMC16 7.5T)* |

 ## Authors
- Authors/maintainer since Jan 2020: Mingyu Woo (Ph.D. Advisor: Andrew. B. Kahng)
- Original open-sourcing of RePlAce: August 2018, by Ilgweon Kang (Ph.D. Advisor: Chung-Kuan Cheng), Lutong Wang (Ph.D. Advisor: Andrew B. Kahng), and Mingyu Woo (Ph.D. Advisor: Andrew B. Kahng).  
- Also thanks to Dr. Jingwei Lu for open-sourcing the previous ePlace-MS/ePlace project from Dr. Jingwei Lu. 

- Paper reference:
  - C.-K. Cheng, A. B. Kahng, I. Kang and L. Wang, "RePlAce: Advancing Solution Quality and Routability Validation in Global Placement", IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, 38(9) (2019), pp. 1717-1730.
  - J. Lu, P. Chen, C.-C. Chang, L. Sha, D. J.-H. Huang, C.-C. Teng and C.-K. Cheng, "ePlace: Electrostatics based Placement using Fast Fourier Transform and Nesterov's Method", ACM TODAES 20(2) (2015), article 17.
  - J. Lu, H. Zhuang, P. Chen, H. Chang, C.-C. Chang, Y.-C. Wong, L. Sha, D. J.-H. Huang, Y. Luo, C.-C. Teng and C.-K. Cheng, "ePlace-MS: Electrostatics based Placement for Mixed-Size Circuits", IEEE TCAD 34(5) (2015), pp. 685-698.

- The timing-driven mode has been implemented by Mingyu Woo (only available in [legacy repo in standalone branch](https://github.com/The-OpenROAD-Project/RePlAce/tree/standalone).)
- The routability-driven mode has been implemented by Mingyu Woo.
- Timing-driven mode re-implementation is ongoing with the current clean-code structure.   

## License
* BSD-3-clause License [[Link]](./LICENSE)
