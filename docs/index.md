# Welcome to OpenROAD's documentation!

The OpenROAD ("Foundations and Realization of Open, Accessible Design")
project was launched in June 2018 within the DARPA IDEA program. OpenROAD
aims to bring down the barriers of cost, expertise and unpredictability that
currently block designers' access to hardware implementation in advanced
technologies. The project team UC San Diego is developing a fully autonomous,
open-source tool chain for digital layout generation across die, package
and board, with initial focus on the RTL-to-GDSII phase of system-on-chip
design. Thus, OpenROAD holistically attacks the multiple facets of today's
design cost crisis: engineering resources, design tool licenses, project
schedule, and risk.

The IDEA program targets no-human-in-loop (NHIL) design, with 24-hour
turnaround time and zero loss of power-performance-area (PPA) design quality.

The NHIL target requires tools to adapt and auto-tune successfully to flow
completion without or with significantly minimal human intervention. Machine
intelligence augments human expertise through efficient modeling and
prediction of flow outcomes during layout generation.

The 24-hour runtime target implies that problems must be strategically
decomposed throughout the design process, with clustered and partitioned
subproblems being solved and recomposed through intelligent distribution
and management of computational resources. This ensures that the NHIL design
optimization is performed within its available `[threads * hours]` "box" of
resources. Decomposition that enables parallel and distributed search over
cloud resources incurs a quality-of-results loss, but this is subsequently
recovered through improved flow predictability and enhanced optimization.

For a technical description of the OpenROAD flow, please refer to our DAC paper:
[Toward an Open-Source Digital Flow: First Learnings from the OpenROAD Project](https://vlsicad.ucsd.edu/Publications/Conferences/371/c371.pdf).
The paper is also available from [ACM Digital Library](https://dl.acm.org/doi/10.1145/3316781.3326334).
Other publications and presentations are
[linked here](https://theopenroadproject.org/publications/).

## Code of conduct

Please read our code of conduct [here](main/CODE_OF_CONDUCT.md).

## Documentation

The OpenROAD Project has two releases:

### [Application](https://github.com/The-OpenROAD-Project/OpenROAD)

The application is a standalone binary capable to perform chip design
from the floorplan to detailed route with metal fill.

See documentation for the [application here](main/README.md).

### [Flow](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts)

The flow is a set of integrated scripts that allow for RTL-to-GDSII flow
using open-source tools.

See documentation for the [flow here](user/GettingStarted.md).

## How to contribute

If you are willing to **contribute**, see the
[Getting Involved](contrib/GettingInvolved.md) section.

If you are a **developer** with EDA background, learn more about how you
can use OpenROAD as the infrastructure for your tools in the
[Developer Guide](contrib/DeveloperGuide.md) section.

## How to get in touch

We maintain the following channels for communication:

-   Project homepage and news: <https://theopenroadproject.org>
-   Twitter: <https://twitter.com/OpenROAD_EDA>
-   Issues and bugs:
    -   OpenROAD: <https://github.com/The-OpenROAD-Project/OpenROAD/issues>
    -   OpenROAD Flow: <https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/issues>
-   Discussions:
    -   OpenROAD: <https://github.com/The-OpenROAD-Project/OpenROAD/discussions>
    -   OpenROAD Flow: <https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/discussions>

See also our [FAQs](user/FAQS.md).

## Site Map

```{tableofcontents}
```
