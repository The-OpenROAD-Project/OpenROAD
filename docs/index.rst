.. OpenROAD documentation master file, created by
   sphinx-quickstart on Mon Feb 17 12:17:21 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to OpenROAD's documentation!
====================================

The OpenROAD ("Foundations and Realization of Open, Accessible Design")
project was launched in June 2018 within the DARPA IDEA program. OpenROAD
aims to bring down the barriers of cost, expertise and unpredictability that
currently block designers' access to hardware implementation in advanced
technologies. The project team (Qualcomm, Arm and multiple universities and
partners, led by UC San Diego) is developing a fully autonomous, open-source
tool chain for digital layout generation across die, package and board,
with initial focus on the RTL-to-GDSII phase of system-on-chip design. Thus,
OpenROAD holistically attacks the multiple facets of today's design cost
crisis: engineering resources, design tool licenses, project schedule,
and risk.

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

For a technical description of the OpenROAD flow, please refer to our
DAC paper: `Toward an Open-Source Digital Flow: First Learnings from the
OpenROAD Project`_. The paper is also available from `ACM Digital Library`_.
Other publications and presentations are `linked here`_.

How to navigate this documentation
-----------------------------------

* If you are a **user**, start with the :doc:`Getting Started <user/GettingStarted>` guide, and then move on to the :doc:`User Guide <user/UserGuide>`.
* If you are willing to **contribute**, see the :doc:`Getting Involved <contrib/GettingInvolved>` section.
* If you are a **developer** with EDA background, learn more about how you can use OpenROAD as the infrastructure for your tools in the :doc:`Developer Guide <contrib/DeveloperGuide>` section.

See :doc:`FAQs <user/FAQS>` and :doc:`Capabilities/Limitations <user/ToolLimitations>` for relevant background on the project.

How to get in touch
--------------------

We maintain the following channels for communication:

+ Project homepage and news: https://theopenroadproject.org
+ Twitter: https://twitter.com/OpenROAD_EDA
+ Issues and bugs: https://github.com/The-OpenROAD-Project/OpenROAD/issues
+ Gitter Community: https://gitter.im/The-OpenROAD-Project/community
+ Inquiries: openroad@eng.ucsd.edu

Site Map
--------

.. toctree::
   :maxdepth: 3
   :caption: Contents:

   user/GettingStarted
   user/UserGuide
   user/ToolLimitations
   contrib/DeveloperGuide
   contrib/CodingPractices
   contrib/DatabaseMath
   contrib/GettingInvolved
   contrib/Logger
   contrib/Metrics
   user/FAQS


.. _`Toward an Open-Source Digital Flow: First Learnings from the OpenROAD Project`: https://vlsicad.ucsd.edu/Publications/Conferences/371/c371.pdf
.. _`ACM Digital Library`: https://dl.acm.org/doi/10.1145/3316781.3326334
.. _`linked here`: https://theopenroadproject.org/publications/
