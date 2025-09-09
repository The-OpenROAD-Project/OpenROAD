FastRoute

Min Pan, Yue Xu, Yanheng Zhang, Chris Chu

Contacts: yuexu@iastate.edu

Introduction

FastRoute is a global routing tool for VLSI back-end design. It  is based on sequential rip-up and re-route (RRR) and a lot of novel techniques. FastRoute 1.0 first uses FLUTE to construct congestion-driven Steiner trees, which will later undergo the edge shifting process to optimize tree structure to reduce congestion. It then uses pattern routing and maze routing with logistic function based cost function to solve the congestion problem. FastRoute 2.0 proposed monotonic routing and multi-source multi-sink maze routing techniques to enhance the capability to reduce congestion. FastRoute 3.0 introduced the virtual capacity technique to adaptively change the capacity associated with each global edge to divert wire usage from highly congested regions to less congested regions. FastRoute 4.0 proposed via-aware Steiner tree, 3-bend routing and a delicate layer assignment algorithm to effectively reduce via count while maintaining outstanding congestion reduction capability. FastRoute 4.1 simplifies the way the virtual capacities are updated and applies a single set of tuning parameters to all benchmark circuits.

Literature

1. Chris Chu and Yiu-Chung Wong, FLUTE: Fast Lookup Table Based Rectilinear Steiner Minimal Tree Algorithm for VLSI Design. In IEEE Transactions on Computer-Aided Design, vol. 27, no. 1, pages 70-83, January 2008.
2. Min Pan and Chris Chu, FastRoute: A Step to Integrate Global Routing into Placement. IEEE/ACM International Conference on Computer-Aided Design, pages 464-471, 2006.
3. Min Pan and Chris Chu, FastRoute 2.0: A High-quality and Efficient Global Router. Asian and South Pacific Design Automation Conference, pages 250-255, 2007.
4. Yanheng Zhang, Yue Xu and Chris Chu, FastRoute 3.0: A Fast and High Quality Global Router Based on Virtual Capacity. IEEE/ACM International Conference on Computer-Aided Design, pages 344-349, 2008.
5. Yue Xu, Yanheng Zhang and Chris Chu. "FastRoute 4.0: Global Router with Efficient Via Minimization. Asian and South Pacific Design Automation Conference, pages 576-581, 2009.
6. Min Pan, Yue Xu, Yanheng Zhang and Chris Chu. "FastRoute: An Efficient and High-Quality Global Router. VLSI Design, Article ID 608362, 2012.
