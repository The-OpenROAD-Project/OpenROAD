.. OpenROAD documentation master file, created by
   sphinx-quickstart on Mon Feb 17 12:17:21 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to OpenROAD's documentation!
====================================

The OpenROAD ("Foundations and Realization of Open, Accessible Design") project
was launched in June 2018 within the DARPA IDEA program. OpenROAD aims to bring
down the barriers of cost, expertise and unpredictability that currently block
designers' access to hardware implementation in advanced technologies. The
project team (Qualcomm, Arm and multiple universities and partners, led by UC
San Diego) is developing a fully autonomous, open-source tool chain for digital
layout generation across die, package and board, with initial focus on the
RTL-to-GDSII phase of system-on-chip design. Thus, OpenROAD holistically attacks
the multiple facets of today's design cost crisis: engineering resources, design
tool licenses, project schedule, and risk.

The IDEA program targets no-human-in-loop (NHIL) design, with 24-hour turnaround
time and eventual zero loss of power-performance-area (PPA) design quality. No
humans means that tools must adapt and self-tune, and never get stuck: thus,
machine intelligence must replace today's human intelligence within the layout
generation process. 24 hours means that problems must be aggressively decomposed
into bite-sized subproblems for the design process to remain within the schedule
constraint. Eventual zero loss of PPA quality requires parallel and distributed
search to recoup the solution quality lost by problem decomposition.

For a technical description of the OpenROAD flow, please refer to our DAC paper:
`Toward an Open-Source Digital Flow: First Learnings from the OpenROAD
Project`_. Also, available from ACM Digital Library (`doi:10.1145/3316781.3326334`_)

How to navigate this documentation
-----------------------------------

* If you are a **user**, start with the `Getting Started`_ guide, and then move on to the `User Guide`_.
* If you are willing to **contribute**, see the `Getting Involved`_ section.
* If you are a **developer** with EDA background, learn more about how you can use OpenROAD as the infrastructure for your tools in the `Developer Guide`_ section.

See `FAQs`_ and `Capabilities/Limitations`_ for relevant background on the project.

How to get in touch
--------------------

We maintain the following channels for communication:

+ Project homepage and news: https://theopenroadproject.org
+ Twitter: https://twitter.com/OpenROAD_EDA
+ Issues and bugs: https://github.com/The-OpenROAD-Project/OpenROAD/issues
+ Gitter Community: https://gitter.im/The-OpenROAD-Project/community
+ Inquiries: openroad@eng.ucsd.edu

.. toctree::
   :maxdepth: 3
   :caption: Contents:

   user/GettingStarted
   user/UserGuide
   user/ToolLimitations
   contrib/GettingInvolved
   contrib/DeveloperGuide
   contrib/DatabaseMath
   user/FAQs


.. _OpenDB: https://github.com/The-OpenROAD-Project/OpenDB
.. _OpenSTA: https://github.com/The-OpenROAD-Project/OpenSTA
.. _`Toward an Open-Source Digital Flow: First Learnings from the OpenROAD Project`: https://vlsicad.ucsd.edu/Publications/Conferences/371/c371.pdf
.. _`doi:10.1145/3316781.3326334`: https://dl.acm.org/citation.cfm?id=3326334
.. _`Getting Started`: user/getting-started.html
.. _`User Guide`: user/user-guide.html
.. _`Getting Involved`: contrib/getting-involved.html
.. _`Developer Guide`: contrib/developer-guide.html
.. _`FAQs`: user/faqs.html
.. _`Capabilities/Limitations`: user/tool-limitations.html
